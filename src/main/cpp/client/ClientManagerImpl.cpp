#include "ClientManagerImpl.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

#include "google/rpc/code.pb.h"

#include "InvocationContext.h"
#include "LogInterceptor.h"
#include "LogInterceptorFactory.h"
#include "LoggerImpl.h"
#include "MessageAccessor.h"
#include "MetadataConstants.h"
#include "MixAll.h"
#include "Partition.h"
#include "Protocol.h"
#include "RpcClient.h"
#include "RpcClientImpl.h"
#include "TlsHelper.h"
#include "UtilAll.h"
#include "grpcpp/create_channel.h"
#include "rocketmq/ErrorCode.h"
#include "rocketmq/MQMessageExt.h"

ROCKETMQ_NAMESPACE_BEGIN

ClientManagerImpl::ClientManagerImpl(std::string resource_namespace)
    : resource_namespace_(std::move(resource_namespace)), state_(State::CREATED),
      completion_queue_(std::make_shared<CompletionQueue>()),
      callback_thread_pool_(absl::make_unique<ThreadPoolImpl>(std::thread::hardware_concurrency())),
      latency_histogram_("Message-Latency", 11) {
  spdlog::set_level(spdlog::level::trace);
  assignLabels(latency_histogram_);
  server_authorization_check_config_ = std::make_shared<grpc::experimental::TlsServerAuthorizationCheckConfig>(
      std::make_shared<TlsServerAuthorizationChecker>());

  // Make use of encryption only at the moment.
  std::vector<grpc::experimental::IdentityKeyCertPair> identity_key_cert_list;
  grpc::experimental::IdentityKeyCertPair pair{};
  pair.private_key = TlsHelper::client_private_key;
  pair.certificate_chain = TlsHelper::client_certificate_chain;

  identity_key_cert_list.emplace_back(pair);
  certificate_provider_ =
      std::make_shared<grpc::experimental::StaticDataCertificateProvider>(TlsHelper::CA, identity_key_cert_list);
  tls_channel_credential_options_.set_server_verification_option(GRPC_TLS_SKIP_ALL_SERVER_VERIFICATION);
  tls_channel_credential_options_.set_certificate_provider(certificate_provider_);
  tls_channel_credential_options_.set_server_authorization_check_config(server_authorization_check_config_);
  tls_channel_credential_options_.watch_root_certs();
  tls_channel_credential_options_.watch_identity_key_cert_pairs();
  channel_credential_ = grpc::experimental::TlsCredentials(tls_channel_credential_options_);
  SPDLOG_INFO("ClientManager[ResourceNamespace={}] created", resource_namespace_);
}

ClientManagerImpl::~ClientManagerImpl() {
  shutdown();
  SPDLOG_INFO("ClientManager[ResourceNamespace={}] destructed", resource_namespace_);
}

void ClientManagerImpl::start() {
  if (State::CREATED != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state: {}", state_.load(std::memory_order_relaxed));
    return;
  }
  state_.store(State::STARTING, std::memory_order_relaxed);

  callback_thread_pool_->start();
  scheduler_.start();

  std::weak_ptr<ClientManagerImpl> client_instance_weak_ptr = shared_from_this();

  auto health_check_functor = [client_instance_weak_ptr]() {
    auto client_instance = client_instance_weak_ptr.lock();
    if (client_instance) {
      client_instance->doHealthCheck();
    }
  };
  health_check_task_id_ = scheduler_.schedule(health_check_functor, HEALTH_CHECK_TASK_NAME, std::chrono::seconds(5),
                                              std::chrono::seconds(5));
  auto heartbeat_functor = [client_instance_weak_ptr]() {
    auto client_instance = client_instance_weak_ptr.lock();
    if (client_instance) {
      client_instance->doHeartbeat();
    }
  };
  heartbeat_task_id_ =
      scheduler_.schedule(heartbeat_functor, HEARTBEAT_TASK_NAME, std::chrono::seconds(1), std::chrono::seconds(10));

  completion_queue_thread_ = std::thread(std::bind(&ClientManagerImpl::pollCompletionQueue, this));

  auto stats_functor_ = [client_instance_weak_ptr]() {
    auto client_instance = client_instance_weak_ptr.lock();
    if (client_instance) {
      client_instance->logStats();
    }
  };
  stats_task_id_ =
      scheduler_.schedule(stats_functor_, STATS_TASK_NAME, std::chrono::seconds(0), std::chrono::seconds(10));
  state_.store(State::STARTED, std::memory_order_relaxed);
}

void ClientManagerImpl::shutdown() {
  SPDLOG_DEBUG("Client instance shutdown");
  if (State::STARTED != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state: {}", state_.load(std::memory_order_relaxed));
    return;
  }
  state_.store(STOPPING, std::memory_order_relaxed);

  callback_thread_pool_->shutdown();

  if (health_check_task_id_) {
    scheduler_.cancel(health_check_task_id_);
  }

  if (heartbeat_task_id_) {
    scheduler_.cancel(heartbeat_task_id_);
  }

  if (stats_task_id_) {
    scheduler_.cancel(stats_task_id_);
  }
  scheduler_.shutdown();

  {
    absl::MutexLock lk(&rpc_clients_mtx_);
    rpc_clients_.clear();
    SPDLOG_DEBUG("CompletionQueue of active clients stopped");
  }

  completion_queue_->Shutdown();
  if (completion_queue_thread_.joinable()) {
    completion_queue_thread_.join();
  }
  SPDLOG_DEBUG("Completion queue thread completes OK");

  state_.store(State::STOPPED, std::memory_order_relaxed);
  SPDLOG_DEBUG("Client instance stopped");
}

void ClientManagerImpl::assignLabels(Histogram& histogram) {
  histogram.labels().emplace_back("[000ms~020ms): ");
  histogram.labels().emplace_back("[020ms~040ms): ");
  histogram.labels().emplace_back("[040ms~060ms): ");
  histogram.labels().emplace_back("[060ms~080ms): ");
  histogram.labels().emplace_back("[080ms~100ms): ");
  histogram.labels().emplace_back("[100ms~120ms): ");
  histogram.labels().emplace_back("[120ms~140ms): ");
  histogram.labels().emplace_back("[140ms~160ms): ");
  histogram.labels().emplace_back("[160ms~180ms): ");
  histogram.labels().emplace_back("[180ms~200ms): ");
  histogram.labels().emplace_back("[200ms~inf): ");
}

void ClientManagerImpl::healthCheck(
    const std::string& target_host, const Metadata& metadata, const HealthCheckRequest& request,
    std::chrono::milliseconds timeout,
    const std::function<void(const std::error_code&, const InvocationContext<HealthCheckResponse>*)>& cb) {
  std::error_code ec;
  auto client = getRpcClient(target_host);
  if (!client) {
    ec = ErrorCode::RequestTimeout;
    cb(ec, nullptr);
    return;
  }

  auto invocation_context = new InvocationContext<HealthCheckResponse>();
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);

  for (const auto& entry : metadata) {
    invocation_context->context.AddMetadata(entry.first, entry.second);
  }

  auto callback = [cb](const InvocationContext<HealthCheckResponse>* ctx) {
    std::error_code ec;
    if (!ctx->status.ok()) {
      ec = ErrorCode::RequestTimeout;
      cb(ec, ctx);
      return;
    }

    const auto& common = ctx->response.common();
    switch (common.status().code()) {
      case google::rpc::Code::OK: {
        cb(ec, ctx);
      } break;
      case google::rpc::Code::UNAUTHENTICATED: {
        SPDLOG_WARN("Unauthenticated: {}", common.status().message());
        ec = ErrorCode::Unauthorized;
        cb(ec, ctx);
      } break;
      case google::rpc::Code::PERMISSION_DENIED: {
        SPDLOG_WARN("PermissionDenied: {}", common.status().message());
        ec = ErrorCode::Forbidden;
        cb(ec, ctx);
      } break;
      case google::rpc::Code::INTERNAL: {
        SPDLOG_WARN("InternalServerError: {}", common.status().message());
        ec = ErrorCode::InternalServerError;
        cb(ec, ctx);
      } break;
      default: {
        SPDLOG_WARN("NotImplemented: please upgrade SDK to latest release");
        ec = ErrorCode::NotImplemented;
        cb(ec, ctx);
      } break;
    }
  };

  invocation_context->callback = callback;
  client->asyncHealthCheck(request, invocation_context);
}

void ClientManagerImpl::doHealthCheck() {
  SPDLOG_DEBUG("Start to perform health check for inactive clients");
  if (State::STARTED != state_.load(std::memory_order_relaxed) &&
      State::STARTING != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state={}.", state_.load(std::memory_order_relaxed));
    return;
  }

  auto&& rpc_clients_removed = cleanOfflineRpcClients();

  std::vector<std::shared_ptr<Client>> clients;
  {
    absl::MutexLock lk(&clients_mtx_);
    for (auto& item : clients_) {
      auto client = item.lock();
      if (client && client->active()) {
        clients.emplace_back(std::move(client));
      }
    }
  }

  if (!rpc_clients_removed.empty()) {
    for (auto& client : clients) {
      client->onRemoteEndpointRemoval(rpc_clients_removed);
    }
  }

  for (auto& client : clients) {
    client->healthCheck();
  }
  SPDLOG_DEBUG("Health check completed");
}

std::vector<std::string> ClientManagerImpl::cleanOfflineRpcClients() {
  absl::flat_hash_set<std::string> hosts;
  {
    absl::MutexLock lk(&clients_mtx_);
    for (const auto& item : clients_) {
      std::shared_ptr<Client> client = item.lock();
      if (!client) {
        continue;
      }
      client->endpointsInUse(hosts);
    }
  }

  std::vector<std::string> removed;
  {
    absl::MutexLock lk(&rpc_clients_mtx_);
    for (auto it = rpc_clients_.begin(); it != rpc_clients_.end();) {
      std::string host = it->first;
      if (it->second->needHeartbeat() && !hosts.contains(host)) {
        SPDLOG_INFO("Removed RPC client whose peer is offline. RemoteHost={}", host);
        removed.push_back(host);
        rpc_clients_.erase(it++);
      } else {
        it++;
      }
    }
  }

  return removed;
}

void ClientManagerImpl::heartbeat(const std::string& target_host, const Metadata& metadata,
                                  const HeartbeatRequest& request, std::chrono::milliseconds timeout,
                                  const std::function<void(const std::error_code&, const HeartbeatResponse&)>& cb) {
  auto client = getRpcClient(target_host, true);
  if (!client) {
    return;
  }

  auto invocation_context = new InvocationContext<HeartbeatResponse>();
  invocation_context->remote_address = target_host;
  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  auto callback = [cb](const InvocationContext<HeartbeatResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to send heartbeat to target_host={}. gRPC code: {}, message: {}",
                  invocation_context->remote_address, invocation_context->status.error_code(),
                  invocation_context->status.error_message());
      std::error_code ec = ErrorCode::RequestTimeout;
      cb(ec, invocation_context->response);
      return;
    }

    const auto& common = invocation_context->response.common();
    std::error_code ec;
    switch (common.status().code()) {
      case google::rpc::Code::OK: {
        cb(ec, invocation_context->response);
      } break;
      case google::rpc::Code::UNAUTHENTICATED: {
        SPDLOG_WARN("Unauthenticated: {}", common.status().message());
        ec = ErrorCode::Unauthorized;
        cb(ec, invocation_context->response);
      } break;
      case google::rpc::Code::PERMISSION_DENIED: {
        SPDLOG_WARN("PermissionDenied: {}", common.status().message());
        ec = ErrorCode::Forbidden;
        cb(ec, invocation_context->response);
      } break;
      case google::rpc::Code::INVALID_ARGUMENT: {
        SPDLOG_WARN("InvalidArgument: {}", common.status().message());
        ec = ErrorCode::BadRequest;
        cb(ec, invocation_context->response);
      } break;
      case google::rpc::Code::INTERNAL: {
        SPDLOG_WARN("InternalServerError: {}", common.status().message());
        ec = ErrorCode::InternalServerError;
        cb(ec, invocation_context->response);
      } break;
      default: {
        SPDLOG_WARN("NotImplemented: Please upgrade SDK to latest release");
      } break;
    }
  };

  invocation_context->callback = callback;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);
  client->asyncHeartbeat(request, invocation_context);
}

void ClientManagerImpl::doHeartbeat() {
  if (State::STARTED != state_.load(std::memory_order_relaxed) &&
      State::STARTING != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state={}.", state_.load(std::memory_order_relaxed));
    return;
  }

  std::vector<std::shared_ptr<Client>> clients;
  {
    absl::MutexLock lk(&clients_mtx_);
    for (const auto& item : clients_) {
      auto client = item.lock();
      if (client && client->active()) {
        clients.emplace_back(std::move(client));
      }
    }
  }

  for (auto& client : clients) {
    client->heartbeat();
  }
}

void ClientManagerImpl::pollCompletionQueue() {
  while (State::STARTED == state_.load(std::memory_order_relaxed) ||
         State::STARTING == state_.load(std::memory_order_relaxed)) {
    bool ok = false;
    void* opaque_invocation_context;
    while (completion_queue_->Next(&opaque_invocation_context, &ok)) {
      auto invocation_context = static_cast<BaseInvocationContext*>(opaque_invocation_context);
      if (!ok) {
        // the call is dead
        SPDLOG_WARN("CompletionQueue#Next assigned ok false, indicating the call is dead");
      }
      auto callback = [invocation_context, ok]() { invocation_context->onCompletion(ok); };
      callback_thread_pool_->submit(callback);
    }
    SPDLOG_INFO("CompletionQueue is fully drained and shut down");
  }
  SPDLOG_INFO("pollCompletionQueue completed and quit");
}

bool ClientManagerImpl::send(const std::string& target_host, const Metadata& metadata, SendMessageRequest& request,
                             SendCallback* cb) {
  assert(cb);
  SPDLOG_DEBUG("Prepare to send message to {} asynchronously", target_host);
  RpcClientSharedPtr client = getRpcClient(target_host);
  // Invocation context will be deleted in its onComplete() method.
  auto invocation_context = new InvocationContext<SendMessageResponse>();
  invocation_context->remote_address = target_host;
  for (const auto& entry : metadata) {
    invocation_context->context.AddMetadata(entry.first, entry.second);
  }

  const std::string& topic = request.message().topic().name();
  std::weak_ptr<ClientManager> client_manager(shared_from_this());
  auto completion_callback = [topic, cb,
                              client_manager](const InvocationContext<SendMessageResponse>* invocation_context) {
    ClientManagerPtr client_manager_ptr = client_manager.lock();
    if (!client_manager_ptr) {
      return;
    }

    if (State::STARTED != client_manager_ptr->state()) {
      // TODO: Would this leak some memroy?
      return;
    }

    const auto& common = invocation_context->response.common();

    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to send message to {} due to gRPC error. gRPC code: {}, gRPC error message: {}",
                  invocation_context->remote_address, invocation_context->status.error_code(),
                  invocation_context->status.error_message());
      std::error_code ec = ErrorCode::RequestTimeout;
      cb->onFailure(ec);
      return;
    }

    if (invocation_context->status.ok()) {
      switch (invocation_context->response.common().status().code()) {
        case google::rpc::Code::OK: {
          SendResult send_result;
          send_result.setSendStatus(SendStatus::SEND_OK);
          send_result.setMsgId(invocation_context->response.message_id());
          send_result.setTransactionId(invocation_context->response.transaction_id());
          cb->onSuccess(send_result);
        } break;

        case google::rpc::Code::INVALID_ARGUMENT: {
          SPDLOG_WARN("InvalidArgument: {}", common.status().message());
          std::error_code ec = ErrorCode::BadRequest;
          cb->onFailure(ec);
        } break;
        case google::rpc::Code::UNAUTHENTICATED: {
          SPDLOG_WARN("Unauthenticated: {}", common.status().message());
          std::error_code ec = ErrorCode::Unauthorized;
          cb->onFailure(ec);
        } break;
        case google::rpc::Code::PERMISSION_DENIED: {
          SPDLOG_WARN("PermissionDenied: {}", common.status().message());
          std::error_code ec = ErrorCode::Forbidden;
          cb->onFailure(ec);
        } break;
        case google::rpc::Code::INTERNAL: {
          SPDLOG_WARN("InternalServerError: {}", common.status().message());
          std::error_code ec = ErrorCode::InternalServerError;
          cb->onFailure(ec);
        } break;
        default: {
          SPDLOG_WARN("Unsupported status code. Check and upgrade SDK to the latest");
          std::error_code ec = ErrorCode::NotImplemented;
          cb->onFailure(ec);
        } break;
      }
    }
  };

  invocation_context->callback = completion_callback;
  client->asyncSend(request, invocation_context);
  return true;
}

/**
 * @brief Create a gRPC channel to target host.
 *
 * @param target_host
 * @return std::shared_ptr<grpc::Channel>
 */
std::shared_ptr<grpc::Channel> ClientManagerImpl::createChannel(const std::string& target_host) {
  std::vector<std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>> interceptor_factories;
  interceptor_factories.emplace_back(absl::make_unique<LogInterceptorFactory>());
  auto channel = grpc::experimental::CreateCustomChannelWithInterceptors(
      target_host, channel_credential_, channel_arguments_, std::move(interceptor_factories));
  return channel;
}

RpcClientSharedPtr ClientManagerImpl::getRpcClient(const std::string& target_host, bool need_heartbeat) {
  std::shared_ptr<RpcClient> client;
  {
    absl::MutexLock lock(&rpc_clients_mtx_);
    auto search = rpc_clients_.find(target_host);
    if (search == rpc_clients_.end() || !search->second->ok()) {
      if (search == rpc_clients_.end()) {
        SPDLOG_INFO("Create a RPC client to {}", target_host.data());
      } else if (!search->second->ok()) {
        SPDLOG_INFO("Prior RPC client to {} is not OK. Re-create one", target_host);
      }
      std::vector<std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>> interceptor_factories;
      interceptor_factories.emplace_back(absl::make_unique<LogInterceptorFactory>());
      auto channel = grpc::experimental::CreateCustomChannelWithInterceptors(
          target_host, channel_credential_, channel_arguments_, std::move(interceptor_factories));
      client = std::make_shared<RpcClientImpl>(completion_queue_, channel, need_heartbeat);
      rpc_clients_.insert_or_assign(target_host, client);
    } else {
      client = search->second;
    }
  }

  if (need_heartbeat && !client->needHeartbeat()) {
    client->needHeartbeat(need_heartbeat);
  }

  return client;
}

void ClientManagerImpl::addRpcClient(const std::string& target_host, const RpcClientSharedPtr& client) {
  {
    absl::MutexLock lock(&rpc_clients_mtx_);
    rpc_clients_.insert_or_assign(target_host, client);
  }
}

void ClientManagerImpl::cleanRpcClients() {
  absl::MutexLock lk(&rpc_clients_mtx_);
  rpc_clients_.clear();
}

SendResult ClientManagerImpl::processSendResponse(const MQMessageQueue& message_queue,
                                                  const SendMessageResponse& response) {
  if (google::rpc::Code::OK != response.common().status().code()) {
    THROW_MQ_EXCEPTION(MQClientException, response.common().DebugString(), response.common().status().code());
  }
  SendResult send_result;
  send_result.setSendStatus(SendStatus::SEND_OK);
  send_result.setMsgId(response.message_id());
  send_result.setQueueOffset(-1);
  send_result.setMessageQueue(message_queue);
  send_result.setTransactionId(response.transaction_id());
  return send_result;
}

void ClientManagerImpl::addClientObserver(std::weak_ptr<Client> client) {
  absl::MutexLock lk(&clients_mtx_);
  clients_.emplace_back(std::move(client));
}

void ClientManagerImpl::resolveRoute(const std::string& target_host, const Metadata& metadata,
                                     const QueryRouteRequest& request, std::chrono::milliseconds timeout,
                                     const std::function<void(const std::error_code&, const TopicRouteDataPtr&)>& cb) {

  RpcClientSharedPtr client = getRpcClient(target_host, false);
  if (!client) {
    SPDLOG_WARN("Failed to create RPC client for name server[host={}]", target_host);
    std::error_code ec = ErrorCode::RequestTimeout;
    cb(ec, nullptr);
    return;
  }

  auto invocation_context = new InvocationContext<QueryRouteResponse>();
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);
  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  auto callback = [cb](const InvocationContext<QueryRouteResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to send query route request to server[host={}]. Reason: {}",
                  invocation_context->remote_address, invocation_context->status.error_message());
      std::error_code ec = ErrorCode::RequestTimeout;
      cb(ec, nullptr);
      return;
    }

    std::error_code ec;
    const auto& common = invocation_context->response.common();
    switch (common.status().code()) {
      case google::rpc::Code::OK: {
        auto& partitions = invocation_context->response.partitions();
        std::vector<Partition> topic_partitions;
        for (const auto& partition : partitions) {
          Topic t(partition.topic().resource_namespace(), partition.topic().name());

          auto& broker = partition.broker();
          AddressScheme scheme = AddressScheme::IPv4;
          switch (broker.endpoints().scheme()) {
            case rmq::AddressScheme::IPv4:
              scheme = AddressScheme::IPv4;
              break;
            case rmq::AddressScheme::IPv6:
              scheme = AddressScheme::IPv6;
              break;
            case rmq::AddressScheme::DOMAIN_NAME:
              scheme = AddressScheme::DOMAIN_NAME;
              break;
            default:
              break;
          }

          std::vector<Address> addresses;
          for (const auto& address : broker.endpoints().addresses()) {
            addresses.emplace_back(Address{address.host(), address.port()});
          }
          ServiceAddress service_address(scheme, addresses);
          Broker b(partition.broker().name(), partition.broker().id(), service_address);

          Permission permission = Permission::READ_WRITE;
          switch (partition.permission()) {
            case rmq::Permission::READ:
              permission = Permission::READ;
              break;

            case rmq::Permission::WRITE:
              permission = Permission::WRITE;
              break;
            case rmq::Permission::READ_WRITE:
              permission = Permission::READ_WRITE;
              break;
            default:
              break;
          }
          Partition topic_partition(t, partition.id(), permission, std::move(b));
          topic_partitions.emplace_back(std::move(topic_partition));
        }
        auto ptr =
            std::make_shared<TopicRouteData>(std::move(topic_partitions), invocation_context->response.DebugString());
        cb(ec, ptr);
      } break;
      case google::rpc::Code::UNAUTHENTICATED: {
        SPDLOG_WARN("Unauthenticated: {}", common.status().message());
        ec = ErrorCode::Unauthorized;
        cb(ec, nullptr);
      } break;
      case google::rpc::Code::PERMISSION_DENIED: {
        SPDLOG_WARN("PermissionDenied: {}", common.status().message());
        ec = ErrorCode::Forbidden;
        cb(ec, nullptr);
      } break;
      case google::rpc::Code::INVALID_ARGUMENT: {
        SPDLOG_WARN("InvalidArgument: {}", common.status().message());
        ec = ErrorCode::BadRequest;
        cb(ec, nullptr);
      } break;
      case google::rpc::Code::NOT_FOUND: {
        SPDLOG_WARN("NotFound: {}", common.status().message());
        ec = ErrorCode::NotFound;
        cb(ec, nullptr);
      } break;
      case google::rpc::Code::INTERNAL: {
        SPDLOG_WARN("InternalServerError: {}", common.status().message());
        ec = ErrorCode::InternalServerError;
        cb(ec, nullptr);
      } break;
      default: {
        SPDLOG_WARN("NotImplement: Please upgrade to latest SDK release");
        ec = ErrorCode::NotImplemented;
        cb(ec, nullptr);
      } break;
    }
  };
  invocation_context->callback = callback;
  client->asyncQueryRoute(request, invocation_context);
}

void ClientManagerImpl::queryAssignment(
    const std::string& target, const Metadata& metadata, const QueryAssignmentRequest& request,
    std::chrono::milliseconds timeout,
    const std::function<void(const std::error_code&, const QueryAssignmentResponse&)>& cb) {
  SPDLOG_DEBUG("Prepare to send query assignment request to broker[address={}]", target);
  std::shared_ptr<RpcClient> client = getRpcClient(target);

  auto callback = [&, cb](const InvocationContext<QueryAssignmentResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to query assignment. Reason: {}", invocation_context->status.error_message());
      std::error_code ec = ErrorCode::RequestTimeout;
      cb(ec, invocation_context->response);
      return;
    }

    const auto& common = invocation_context->response.common();
    std::error_code ec;
    switch (common.status().code()) {
      case google::rpc::Code::OK: {
        SPDLOG_DEBUG("Query assignment OK");
      } break;
      case google::rpc::Code::UNAUTHENTICATED: {
        SPDLOG_WARN("Unauthenticated: {}", common.status().message());
        ec = ErrorCode::Unauthorized;
      } break;
      case google::rpc::Code::PERMISSION_DENIED: {
        SPDLOG_WARN("PermissionDenied: {}", common.status().message());
        ec = ErrorCode::Forbidden;
      } break;
      case google::rpc::Code::INVALID_ARGUMENT: {
        SPDLOG_WARN("InvalidArgument: {}", common.status().message());
        ec = ErrorCode::BadRequest;
      } break;
      case google::rpc::Code::INTERNAL: {
        SPDLOG_WARN("InternalServerError: {}", common.status().message());
        ec = ErrorCode::InternalServerError;
      } break;
      default: {
        SPDLOG_WARN("NotImplemented: please upgrade SDK to latest release");
        ec = ErrorCode::NotImplemented;
      } break;
    }
    cb(ec, invocation_context->response);
  };

  auto invocation_context = new InvocationContext<QueryAssignmentResponse>();
  invocation_context->remote_address = target;
  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);
  invocation_context->callback = callback;
  client->asyncQueryAssignment(request, invocation_context);
}

void ClientManagerImpl::receiveMessage(const std::string& target_host, const Metadata& metadata,
                                       const ReceiveMessageRequest& request, std::chrono::milliseconds timeout,
                                       const std::shared_ptr<ReceiveMessageCallback>& cb) {
  SPDLOG_DEBUG("Prepare to pop message from {} asynchronously. Request: {}", target_host, request.DebugString());
  RpcClientSharedPtr client = getRpcClient(target_host);

  auto invocation_context = new InvocationContext<ReceiveMessageResponse>();
  invocation_context->remote_address = target_host;
  if (!metadata.empty()) {
    for (const auto& item : metadata) {
      invocation_context->context.AddMetadata(item.first, item.second);
    }
  }
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);

  auto callback = [this, cb](const InvocationContext<ReceiveMessageResponse>* invocation_context) {
    if (invocation_context->status.ok()) {
      SPDLOG_DEBUG("Received pop response through gRPC from brokerAddress={}", invocation_context->remote_address);
      const auto& common = invocation_context->response.common();
      switch (common.status().code()) {
        case google::rpc::Code::OK: {
          ReceiveMessageResult receive_result;
          this->processPopResult(invocation_context->context, invocation_context->response, receive_result,
                                 invocation_context->remote_address);
          cb->onSuccess(receive_result);
        } break;

        case google::rpc::Code::UNAUTHENTICATED: {
          SPDLOG_WARN("Unauthenticated: {}", common.status().message());
          std::error_code ec = ErrorCode::Unauthorized;
          cb->onFailure(ec);
        } break;

        case google::rpc::Code::PERMISSION_DENIED: {
          SPDLOG_WARN("PermissionDenied: {}", common.status().message());
          std::error_code ec = ErrorCode::Forbidden;
          cb->onFailure(ec);
        } break;

        case google::rpc::Code::INVALID_ARGUMENT: {
          SPDLOG_WARN("InvalidArgument: {}", common.status().message());
          std::error_code ec = ErrorCode::BadRequest;
          cb->onFailure(ec);
        } break;

        case google::rpc::Code::DEADLINE_EXCEEDED: {
          SPDLOG_WARN("DeadlineExceeded: {}", common.status().message());
          std::error_code ec = ErrorCode::GatewayTimeout;
          cb->onFailure(ec);
        } break;

        case google::rpc::Code::INTERNAL: {
          SPDLOG_WARN("IntervalServerError: {}", common.status().message());
          std::error_code ec = ErrorCode::InternalServerError;
          cb->onFailure(ec);
        } break;
        default: {
          SPDLOG_WARN("Unsupported code. Please upgrade to use the latest release");
          std::error_code ec = ErrorCode::NotImplemented;
          cb->onFailure(ec);
        } break;
      }

    } else {
      SPDLOG_WARN("Failed to pop messages through GRPC from {}, gRPC code: {}, gRPC error message: {}",
                  invocation_context->remote_address, invocation_context->status.error_code(),
                  invocation_context->status.error_message());
      std::error_code ec = ErrorCode::RequestTimeout;
      cb->onFailure(ec);
    }
  };
  invocation_context->callback = callback;
  client->asyncReceive(request, invocation_context);
}

void ClientManagerImpl::processPopResult(const grpc::ClientContext& client_context,
                                         const ReceiveMessageResponse& response, ReceiveMessageResult& result,
                                         const std::string& target_host) {
  // process response to result
  ReceiveMessageStatus status;
  switch (response.common().status().code()) {
    case google::rpc::Code::OK: {
      status = ReceiveMessageStatus::OK;
      break;
    }
    case google::rpc::Code::RESOURCE_EXHAUSTED: {
      status = ReceiveMessageStatus::RESOURCE_EXHAUSTED;
      SPDLOG_WARN("Too many pop requests in broker. Long polling is full in the broker side. BrokerAddress={}",
                  target_host);
      break;
    }

    case google::rpc::Code::DEADLINE_EXCEEDED: {
      status = ReceiveMessageStatus::DEADLINE_EXCEEDED;
      break;
    }
    case google::rpc::Code::NOT_FOUND: {
      status = ReceiveMessageStatus::NOT_FOUND;
      break;
    }
    default: {
      SPDLOG_WARN("Pop response indicates server-side error. BrokerAddress={}, Reason={}", target_host,
                  response.common().DebugString());
      status = ReceiveMessageStatus::INTERNAL;
      break;
    }
  }

  result.sourceHost(target_host);

  std::vector<MQMessageExt> msg_found_list;
  if (ReceiveMessageStatus::OK == status) {
    for (auto& item : response.messages()) {
      MQMessageExt message_ext;
      MessageAccessor::setTargetEndpoint(message_ext, target_host);
      if (wrapMessage(item, message_ext)) {
        msg_found_list.emplace_back(message_ext);
      } else {
        // TODO: NACK
      }
    }
  }

  result.status(status);
  auto& delivery_timestamp = response.delivery_timestamp();
  timeval tv{};
  tv.tv_sec = delivery_timestamp.seconds();
  tv.tv_usec = delivery_timestamp.nanos() / 1000;
  result.setPopTime(absl::TimeFromTimeval(tv));

  const auto& protobuf_invisible_duration = response.invisible_duration();
  absl::Duration invisible_period =
      absl::Seconds(protobuf_invisible_duration.seconds()) + absl::Nanoseconds(protobuf_invisible_duration.nanos());
  result.invisibleTime(invisible_period);
  result.setMsgFoundList(msg_found_list);
}

void ClientManagerImpl::processPullResult(const grpc::ClientContext& client_context,
                                          const PullMessageResponse& response, ReceiveMessageResult& result,
                                          const std::string& target_host) {
  if (google::rpc::Code::OK != response.common().status().code()) {
    result.status_ = ReceiveMessageStatus::INTERNAL;
    return;
  }
  result.sourceHost(target_host);
  switch (response.common().status().code()) {
    case google::rpc::Code::OK: {
      assert(!response.messages().empty());
      result.status_ = ReceiveMessageStatus::OK;
      for (const auto& item : response.messages()) {
        MQMessageExt message;
        if (!wrapMessage(item, message)) {
          result.status_ = ReceiveMessageStatus::DATA_CORRUPTED;
          return;
        }
        result.messages_.emplace_back(message);
      }
      break;
    }

    case google::rpc::Code::DEADLINE_EXCEEDED: {
      result.status_ = ReceiveMessageStatus::DEADLINE_EXCEEDED;
      break;
    }

    case google::rpc::Code::RESOURCE_EXHAUSTED: {
      result.status_ = ReceiveMessageStatus::RESOURCE_EXHAUSTED;
      break;
    }

    case google::rpc::Code::OUT_OF_RANGE: {
      result.status_ = ReceiveMessageStatus::OUT_OF_RANGE;
      result.next_offset_ = response.next_offset();
      break;
    }
  }
}

State ClientManagerImpl::state() const {
  return state_.load(std::memory_order_relaxed);
}

bool ClientManagerImpl::wrapMessage(const rmq::Message& item, MQMessageExt& message_ext) {
  assert(item.topic().resource_namespace() == resource_namespace_);

  // base
  message_ext.setTopic(item.topic().name());

  const auto& system_attributes = item.system_attribute();

  // Receipt-handle
  MessageAccessor::setReceiptHandle(message_ext, system_attributes.receipt_handle());

  // Tag
  message_ext.setTags(system_attributes.tag());

  // Keys
  std::vector<std::string> keys;
  for (const auto& key : system_attributes.keys()) {
    keys.push_back(key);
  }
  message_ext.setKeys(keys);

  // Message-Id
  MessageAccessor::setMessageId(message_ext, system_attributes.message_id());

  // Validate body digest
  const rmq::Digest& digest = system_attributes.body_digest();
  bool body_digest_match = false;
  if (item.body().empty()) {
    SPDLOG_WARN("Body of message[topic={}, msgId={}] is empty", item.topic().name(),
                item.system_attribute().message_id());
    body_digest_match = true;
  } else {
    switch (digest.type()) {
      case rmq::DigestType::CRC32: {
        std::string checksum;
        bool success = MixAll::crc32(item.body(), checksum);
        if (success) {
          body_digest_match = (digest.checksum() == checksum);
          if (body_digest_match) {
            SPDLOG_DEBUG("Message body CRC32 checksum validation passed.");
          } else {
            SPDLOG_WARN("Body CRC32 checksum validation failed. Actual: {}, expect: {}", checksum, digest.checksum());
          }
        } else {
          SPDLOG_WARN("Failed to calculate CRC32 checksum. Skip.");
        }
        break;
      }
      case rmq::DigestType::MD5: {
        std::string checksum;
        bool success = MixAll::md5(item.body(), checksum);
        if (success) {
          body_digest_match = (digest.checksum() == checksum);
          if (body_digest_match) {
            SPDLOG_DEBUG("MD5 checksum validation passed.");
          } else {
            SPDLOG_WARN("Body MD5 checksum validation failed. Expect: {}, Actual: {}", digest.checksum(), checksum);
          }
        } else {
          SPDLOG_WARN("Failed to calculate MD5 digest. Skip.");
          body_digest_match = true;
        }
        break;
      }
      case rmq::DigestType::SHA1: {
        std::string checksum;
        bool success = MixAll::sha1(item.body(), checksum);
        if (success) {
          body_digest_match = (checksum == digest.checksum());
          if (body_digest_match) {
            SPDLOG_DEBUG("SHA1 checksum validation passed");
          } else {
            SPDLOG_WARN("Body SHA1 checksum validation failed. Expect: {}, Actual: {}", digest.checksum(), checksum);
          }
        } else {
          SPDLOG_WARN("Failed to calculate SHA1 digest. Skip.");
        }
        break;
      }
      default: {
        SPDLOG_WARN("Unsupported message body digest algorithm");
        body_digest_match = true;
        break;
      }
    }
  }

  if (!body_digest_match) {
    SPDLOG_WARN("Message body checksum failed. MsgId={}", system_attributes.message_id());
    // TODO: NACK it immediately
    return false;
  }

  // Body encoding
  switch (system_attributes.body_encoding()) {
    case rmq::Encoding::GZIP: {
      std::string uncompressed;
      UtilAll::uncompress(item.body(), uncompressed);
      message_ext.setBody(uncompressed);
      break;
    }
    case rmq::Encoding::IDENTITY: {
      message_ext.setBody(item.body());
      break;
    }
    default: {
      SPDLOG_WARN("Unsupported encoding algorithm");
      break;
    }
  }

  timeval tv{};

  // Message-type
  MessageType message_type;
  switch (system_attributes.message_type()) {
    case rmq::MessageType::NORMAL:
      message_type = MessageType::NORMAL;
      break;
    case rmq::MessageType::FIFO:
      message_type = MessageType::FIFO;
      break;
    case rmq::MessageType::DELAY:
      message_type = MessageType::DELAY;
      break;
    case rmq::MessageType::TRANSACTION:
      message_type = MessageType::TRANSACTION;
      break;
    default:
      SPDLOG_WARN("Unknown message type. Treat it as normal message");
      message_type = MessageType::NORMAL;
      break;
  }
  MessageAccessor::setMessageType(message_ext, message_type);

  // Born-timestamp
  if (system_attributes.has_born_timestamp()) {
    tv.tv_sec = system_attributes.born_timestamp().seconds();
    tv.tv_usec = system_attributes.born_timestamp().nanos() / 1000;
    auto born_timestamp = absl::TimeFromTimeval(tv);
    MessageAccessor::setBornTimestamp(message_ext, born_timestamp);
  }

  // Born-host
  MessageAccessor::setBornHost(message_ext, system_attributes.born_host());

  // Store-timestamp
  if (system_attributes.has_store_timestamp()) {
    tv.tv_sec = system_attributes.store_timestamp().seconds();
    tv.tv_usec = system_attributes.store_timestamp().nanos() / 1000;
    MessageAccessor::setStoreTimestamp(message_ext, absl::TimeFromTimeval(tv));
  }

  // Store-host
  MessageAccessor::setStoreHost(message_ext, system_attributes.store_host());

  // Process one-of: delivery-timestamp and delay-level.
  switch (system_attributes.timed_delivery_case()) {
    case rmq::SystemAttribute::TimedDeliveryCase::kDelayLevel: {
      message_ext.setDelayTimeLevel(system_attributes.delay_level());
      break;
    }

    case rmq::SystemAttribute::TimedDeliveryCase::kDeliveryTimestamp: {
      tv.tv_sec = system_attributes.delivery_timestamp().seconds();
      tv.tv_usec = system_attributes.delivery_timestamp().nanos();
      MessageAccessor::setDeliveryTimestamp(message_ext, absl::TimeFromTimeval(tv));
      break;
    }

    default:
      break;
  }

  // Partition-id
  MessageAccessor::setQueueId(message_ext, system_attributes.partition_id());

  // Partition-offset
  MessageAccessor::setQueueOffset(message_ext, system_attributes.partition_offset());

  // Invisible-period
  if (system_attributes.has_invisible_period()) {
    absl::Duration invisible_period = absl::Seconds(system_attributes.invisible_period().seconds()) +
                                      absl::Nanoseconds(system_attributes.invisible_period().nanos());
    MessageAccessor::setInvisiblePeriod(message_ext, invisible_period);
  }

  // Delivery attempt
  MessageAccessor::setDeliveryAttempt(message_ext, system_attributes.delivery_attempt());

  // Trace-context
  MessageAccessor::setTraceContext(message_ext, system_attributes.trace_context());

  // Decoded Time-Point
  MessageAccessor::setDecodedTimestamp(message_ext, absl::Now());

  // User-properties
  std::map<std::string, std::string> properties;
  for (const auto& it : item.user_attribute()) {
    properties.insert(std::make_pair(it.first, it.second));
  }
  message_ext.setProperties(properties);

  // Extension
  {
    auto elapsed = static_cast<int32_t>(absl::ToUnixMillis(absl::Now()) - message_ext.getStoreTimestamp());
    if (elapsed >= 0) {
      latency_histogram_.countIn(elapsed / 20);
    }
  }
  return true;
}

Scheduler& ClientManagerImpl::getScheduler() {
  return scheduler_;
}

void ClientManagerImpl::ack(const std::string& target, const Metadata& metadata, const AckMessageRequest& request,
                            std::chrono::milliseconds timeout, const std::function<void(const std::error_code&)>& cb) {
  std::string target_host(target.data(), target.length());
  SPDLOG_DEBUG("Prepare to ack message against {} asynchronously. AckMessageRequest: {}", target_host,
               request.DebugString());
  RpcClientSharedPtr client = getRpcClient(target_host);

  auto invocation_context = new InvocationContext<AckMessageResponse>();
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);

  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  // TODO: Use capture by move and pass-by-value paradigm when C++ 14 is available.
  auto callback = [request, cb](const InvocationContext<AckMessageResponse>* invocation_context) {
    std::error_code ec;
    if (!invocation_context->status.ok()) {
      ec = ErrorCode::RequestTimeout;
      cb(ec);
      return;
    }

    const auto& common = invocation_context->response.common();
    switch (common.status().code()) {
      case google::rpc::Code::OK: {
        SPDLOG_DEBUG("Ack OK. host={}", invocation_context->remote_address);
      } break;
      case google::rpc::Code::UNAUTHENTICATED: {
        SPDLOG_WARN("Unauthenticated: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::Unauthorized;
      } break;
      case google::rpc::Code::PERMISSION_DENIED: {
        SPDLOG_WARN("PermissionDenied: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::Forbidden;
      } break;
      case google::rpc::Code::INVALID_ARGUMENT: {
        SPDLOG_WARN("InvalidArgument: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::BadRequest;
      } break;
      case google::rpc::Code::INTERNAL: {
        SPDLOG_WARN("InternalServerError: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::InternalServerError;
      } break;
      default: {
        SPDLOG_WARN("NotImplement: please upgrade SDK to latest release. host={}", invocation_context->remote_address);
        ec = ErrorCode::NotImplemented;
      } break;
    }
    cb(ec);
  };
  invocation_context->callback = callback;
  client->asyncAck(request, invocation_context);
}

void ClientManagerImpl::nack(const std::string& target_host, const Metadata& metadata,
                             const NackMessageRequest& request, std::chrono::milliseconds timeout,
                             const std::function<void(const std::error_code&)>& completion_callback) {
  RpcClientSharedPtr client = getRpcClient(target_host);
  assert(client);
  auto invocation_context = new InvocationContext<NackMessageResponse>();
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);

  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  auto callback = [completion_callback](const InvocationContext<NackMessageResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to write Nack request to wire. gRPC-code: {}, gRPC-message: {}",
                  invocation_context->status.error_code(), invocation_context->status.error_message());
      std::error_code ec = ErrorCode::RequestTimeout;
      completion_callback(ec);
      return;
    }

    std::error_code ec;
    const auto& common = invocation_context->response.common();
    switch (common.status().code()) {
      case google::rpc::Code::OK: {
        SPDLOG_DEBUG("Nack to {} OK", invocation_context->remote_address);
        break;
      };
      case google::rpc::Code::UNAUTHENTICATED: {
        SPDLOG_WARN("Unauthenticated: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::Unauthorized;
        break;
      }
      case google::rpc::Code::PERMISSION_DENIED: {
        SPDLOG_WARN("PermissionDenied: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::Forbidden;
        break;
      }
      case google::rpc::Code::INTERNAL: {
        SPDLOG_WARN("InternalServerError: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::InternalServerError;
        break;
      }
      default: {
        SPDLOG_WARN("NotImplemented: Please upgrade to latest SDK, host={}", invocation_context->remote_address);
        ec = ErrorCode::NotImplemented;
        break;
      }
    }
    completion_callback(ec);
  };
  invocation_context->callback = callback;
  client->asyncNack(request, invocation_context);
}

void ClientManagerImpl::endTransaction(
    const std::string& target_host, const Metadata& metadata, const EndTransactionRequest& request,
    std::chrono::milliseconds timeout,
    const std::function<void(const std::error_code&, const EndTransactionResponse&)>& cb) {
  RpcClientSharedPtr client = getRpcClient(target_host);
  if (!client) {
    SPDLOG_WARN("No RPC client for {}", target_host);
    EndTransactionResponse response;
    std::error_code ec = ErrorCode::BadRequest;
    cb(ec, response);
    return;
  }

  SPDLOG_DEBUG("Prepare to endTransaction. TargetHost={}, Request: {}", target_host.data(), request.DebugString());

  auto invocation_context = new InvocationContext<EndTransactionResponse>();
  invocation_context->remote_address = target_host;
  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  // Set RPC deadline.
  auto deadline = std::chrono::system_clock::now() + timeout;
  invocation_context->context.set_deadline(deadline);

  auto callback = [target_host, cb](const InvocationContext<EndTransactionResponse>* invocation_context) {
    std::error_code ec;
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to write EndTransaction to wire. gRPC-code: {}, gRPC-message: {}, host={}",
                  invocation_context->status.error_code(), invocation_context->status.error_message(),
                  invocation_context->remote_address);
      ec = ErrorCode::BadRequest;
      cb(ec, invocation_context->response);
      return;
    }

    const auto& common = invocation_context->response.common();
    switch (common.status().code()) {
      case google::rpc::Code::OK: {
        SPDLOG_DEBUG("endTransaction completed OK. Response: {}, host={}", invocation_context->response.DebugString(),
                     invocation_context->remote_address);
      } break;
      case google::rpc::Code::UNAUTHENTICATED: {
        SPDLOG_WARN("Unauthenticated: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::Unauthorized;
      } break;
      case google::rpc::Code::PERMISSION_DENIED: {
        SPDLOG_WARN("PermissionDenied: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::Forbidden;
      } break;
      case google::rpc::INTERNAL: {
        SPDLOG_WARN("InternalServerError: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::InternalServerError;
      } break;
      default: {
        SPDLOG_WARN("NotImplemented: please upgrade SDK to latest release. {}, host={}", common.status().message(),
                    invocation_context->remote_address);
        ec = ErrorCode::NotImplemented;
      }
    }
    cb(ec, invocation_context->response);
  };

  invocation_context->callback = callback;
  client->asyncEndTransaction(request, invocation_context);
}

void ClientManagerImpl::multiplexingCall(
    const std::string& target_host, const Metadata& metadata, const MultiplexingRequest& request,
    std::chrono::milliseconds timeout, const std::function<void(const InvocationContext<MultiplexingResponse>*)>& cb) {
  RpcClientSharedPtr client = getRpcClient(target_host);
  if (!client) {
    SPDLOG_WARN("No RPC client for {}", target_host);
    cb(nullptr);
    return;
  }

  SPDLOG_DEBUG("Prepare to endTransaction. TargetHost={}, Request: {}", target_host.data(), request.DebugString());

  auto invocation_context = new InvocationContext<MultiplexingResponse>();
  invocation_context->remote_address = target_host;
  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  // Set RPC deadline.
  auto deadline = std::chrono::system_clock::now() + timeout;
  invocation_context->context.set_deadline(deadline);

  auto callback = [cb](const InvocationContext<MultiplexingResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to apply multiplexing-call. TargetHost={}, gRPC statusCode={}, errorMessage={}",
                  invocation_context->remote_address, invocation_context->status.error_message(),
                  invocation_context->status.error_message());
      cb(invocation_context);
      return;
    }

    SPDLOG_DEBUG("endTransaction completed OK. Response: {}", invocation_context->response.DebugString());
    cb(invocation_context);
  };
  invocation_context->callback = callback;
  client->asyncMultiplexingCall(request, invocation_context);
}

void ClientManagerImpl::queryOffset(const std::string& target_host, const Metadata& metadata,
                                    const QueryOffsetRequest& request, std::chrono::milliseconds timeout,
                                    const std::function<void(const std::error_code&, const QueryOffsetResponse&)>& cb) {
  auto client = getRpcClient(target_host);
  std::error_code ec;
  if (!client) {
    SPDLOG_WARN("Failed to get/create RPC client for {}", target_host);
    ec = ErrorCode::RequestTimeout;
    QueryOffsetResponse response;
    cb(ec, response);
    return;
  }

  auto invocation_context = new InvocationContext<QueryOffsetResponse>();
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);
  auto callback = [cb](const InvocationContext<QueryOffsetResponse>* invocation_context) {
    std::error_code ec;

    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to write QueryOffset request to wire. gRPC-code: {}, gRPC-message: {}, host={}",
                  invocation_context->status.error_code(), invocation_context->status.error_message(),
                  invocation_context->remote_address);
      ec = ErrorCode::RequestTimeout;
      cb(ec, invocation_context->response);
      return;
    }

    const auto& common = invocation_context->response.common();
    switch (common.status().code()) {
      case google::rpc::Code::OK: {
        SPDLOG_DEBUG("Query offset from server[host={}] OK", invocation_context->remote_address);
        cb(ec, invocation_context->response);
      } break;
      case google::rpc::Code::UNAUTHENTICATED: {
        SPDLOG_WARN("Unauthenticated: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::Unauthorized;
        cb(ec, invocation_context->response);
      } break;
      case google::rpc::Code::PERMISSION_DENIED: {
        SPDLOG_WARN("PermissionDenied: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::Forbidden;
        cb(ec, invocation_context->response);
      } break;
      case google::rpc::Code::INTERNAL: {
        SPDLOG_WARN("InternalServerError: {}, host={}", common.status().message(), invocation_context->remote_address);
        ec = ErrorCode::InternalServerError;
        cb(ec, invocation_context->response);
      } break;
      default: {
        SPDLOG_WARN("NotImplemented: please upgrade SDK to the latest release. host={}",
                    invocation_context->remote_address);
        ec = ErrorCode::NotImplemented;
        cb(ec, invocation_context->response);
      }
    }
  };
  invocation_context->callback = callback;
  client->asyncQueryOffset(request, invocation_context);
}

void ClientManagerImpl::pullMessage(const std::string& target_host, const Metadata& metadata,
                                    const PullMessageRequest& request, std::chrono::milliseconds timeout,
                                    const std::function<void(const InvocationContext<PullMessageResponse>*)>& cb) {
  auto client = getRpcClient(target_host);
  if (!client) {
    cb(nullptr);
    return;
  }

  auto invocation_context = new InvocationContext<PullMessageResponse>();
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);
  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }
  invocation_context->callback = cb;
  client->asyncPull(request, invocation_context);
}

void ClientManagerImpl::forwardMessageToDeadLetterQueue(
    const std::string& target_host, const Metadata& metadata, const ForwardMessageToDeadLetterQueueRequest& request,
    std::chrono::milliseconds timeout,
    const std::function<void(const InvocationContext<ForwardMessageToDeadLetterQueueResponse>*)>& cb) {
  auto client = getRpcClient(target_host);
  if (!client) {
    cb(nullptr);
    return;
  }

  auto invocation_context = new InvocationContext<ForwardMessageToDeadLetterQueueResponse>();
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);

  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  auto callback = [cb](const InvocationContext<ForwardMessageToDeadLetterQueueResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to transmit SendMessageToDeadLetterQueueRequest to {}", invocation_context->remote_address);
      cb(invocation_context);
      return;
    }

    SPDLOG_DEBUG("Received forwardToDeadLetterQueue response from server[host={}]", invocation_context->remote_address);
    cb(invocation_context);
  };
  invocation_context->callback = callback;
  client->asyncForwardMessageToDeadLetterQueue(request, invocation_context);
}

bool ClientManagerImpl::notifyClientTermination(const std::string& target_host, const Metadata& metadata,
                                                const NotifyClientTerminationRequest& request,
                                                std::chrono::milliseconds timeout) {
  auto client = getRpcClient(target_host);
  if (!client) {
    return false;
  }

  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + timeout);
  for (const auto& item : metadata) {
    context.AddMetadata(item.first, item.second);
  }

  NotifyClientTerminationResponse response;
  grpc::Status status = client->notifyClientTermination(&context, request, &response);
  if (!status.ok()) {
    SPDLOG_WARN("Failed to write NotifyClientTermination request to wire. gRPC-code: {}, gRPC-message: {}, host={}",
                status.error_code(), status.error_message(), target_host);
    return false;
  }

  if (google::rpc::Code::OK == response.common().status().code()) {
    SPDLOG_INFO("Notify client termination to {}", target_host);
    return true;
  }

  SPDLOG_WARN("Failed to notify client termination to {}", target_host);
  return false;
}

void ClientManagerImpl::logStats() {
  std::string stats;
  latency_histogram_.reportAndReset(stats);
  SPDLOG_INFO("{}", stats);
}

const char* ClientManagerImpl::HEARTBEAT_TASK_NAME = "heartbeat-task";
const char* ClientManagerImpl::STATS_TASK_NAME = "stats-task";
const char* ClientManagerImpl::HEALTH_CHECK_TASK_NAME = "health-check-task";

ROCKETMQ_NAMESPACE_END