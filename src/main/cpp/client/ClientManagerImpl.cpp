#include "ClientManagerImpl.h"

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
#include <chrono>
#include <google/rpc/code.pb.h>
#include <memory>
#include <utility>
#include <vector>

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
  SPDLOG_INFO("ClientManager[ARN={}] created", resource_namespace_);
}

ClientManagerImpl::~ClientManagerImpl() {
  shutdown();
  SPDLOG_INFO("ClientManager[ARN={}] destructed", resource_namespace_);
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
    const std::function<void(const std::string&, const InvocationContext<HealthCheckResponse>*)>& cb) {
  {
    absl::MutexLock lk(&rpc_clients_mtx_);
    if (!rpc_clients_.contains(target_host)) {
      SPDLOG_WARN("Try to perform health check for {}, which is unknown to client manager", target_host);
      cb(target_host, nullptr);
      return;
    }
  }

  auto client = getRpcClient(target_host);
  assert(client);

  auto invocation_context = new InvocationContext<HealthCheckResponse>();
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);

  for (const auto& entry : metadata) {
    invocation_context->context.AddMetadata(entry.first, entry.second);
  }

  auto callback = [cb](const InvocationContext<HealthCheckResponse>* ctx) { cb(ctx->remote_address, ctx); };

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

  cleanOfflineRpcClients();

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

  for (auto& client : clients) {
    client->healthCheck();
  }
  SPDLOG_DEBUG("Health check completed");
}

void ClientManagerImpl::cleanOfflineRpcClients() {
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

  {
    absl::MutexLock lk(&rpc_clients_mtx_);
    for (auto it = rpc_clients_.begin(); it != rpc_clients_.end();) {
      std::string host = it->first;
      if (it->second->needHeartbeat() && !hosts.contains(host)) {
        SPDLOG_INFO("Removed RPC client whose peer is offline. RemoteHost={}", host);
        rpc_clients_.erase(it++);
      } else {
        it++;
      }
    }
  }
}

void ClientManagerImpl::heartbeat(const std::string& target_host, const Metadata& metadata,
                                  const HeartbeatRequest& request, std::chrono::milliseconds timeout,
                                  const std::function<void(bool, const HeartbeatResponse&)>& cb) {
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
    if (invocation_context->status.ok()) {
      if (google::rpc::Code::OK == invocation_context->response.common().status().code()) {
        SPDLOG_DEBUG("Send heartbeat to target_host={}, gRPC status OK", invocation_context->remote_address);
        cb(true, invocation_context->response);
      } else {
        SPDLOG_WARN("Server[{}] failed to process heartbeat. Reason: {}", invocation_context->remote_address,
                    invocation_context->response.common().DebugString());
        cb(false, invocation_context->response);
      }
    } else {
      SPDLOG_WARN("Failed to send heartbeat to target_host={}. GRPC code: {}, message : {}",
                  invocation_context->remote_address, invocation_context->status.error_code(),
                  invocation_context->status.error_message());
      cb(false, invocation_context->response);
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

#ifdef ENABLE_TRACING
  nostd::shared_ptr<trace::Span> span = nostd::shared_ptr<trace::Span>(nullptr);
  if (trace_) {
    span = getTracer()->StartSpan("SendMessageAsync");

    MQMessage message = context->getMessage();
    span->SetAttribute(TracingUtility::get().topic_, message.getTopic());
    span->SetAttribute(TracingUtility::get().tags_, message.getTags());
    span->SetAttribute(TracingUtility::get().msg_id_, context->getMessageId());

    const std::string& serialized_span_context = TracingUtility::injectSpanContextToTraceParent(span->GetContext());
    request.mutable_message()->mutable_system_attribute()->set_trace_context(serialized_span_context);
  }
#else
  bool span = false;
#endif

  // Invocation context will be deleted in its onComplete() method.
  auto invocation_context = new InvocationContext<SendMessageResponse>();
  invocation_context->remote_address = target_host;
  for (const auto& entry : metadata) {
    invocation_context->context.AddMetadata(entry.first, entry.second);
  }

  const std::string& topic = request.message().topic().name();
  auto completion_callback = [topic, cb, span, this](const InvocationContext<SendMessageResponse>* invocation_context) {
    if (invocation_context->status.ok() &&
        google::rpc::Code::OK == invocation_context->response.common().status().code()) {

#ifdef ENABLE_TRACING
      if (span) {
        span->SetAttribute(TracingUtility::get().success_, true);
        span->End();
      }
#else
      (void)span;
#endif
      SendResult send_result;
      send_result.setMsgId(invocation_context->response.message_id());
      send_result.setQueueOffset(-1);
      MQMessageQueue message_queue;
      message_queue.setQueueId(-1);
      message_queue.setTopic(topic);
      send_result.setMessageQueue(message_queue);
      if (State::STARTED == state_.load(std::memory_order_relaxed)) {
        cb->onSuccess(send_result);
      } else {
        SPDLOG_INFO("Client instance has stopped, state={}. Ignore send result {}",
                    state_.load(std::memory_order_relaxed), send_result.getMsgId());
      }
    } else {

#ifdef ENABLE_TRACING
      if (span) {
        span->SetAttribute(TracingUtility::get().success_, false);
        span->End();
      }
#endif

      if (!invocation_context->status.ok()) {
        SPDLOG_WARN("Failed to send message to {} due to gRPC error. gRPC code: {}, gRPC error message: {}",
                    invocation_context->remote_address, invocation_context->status.error_code(),
                    invocation_context->status.error_message());
      }
      std::string msg;
      msg.append("gRPC code: ")
          .append(std::to_string(invocation_context->status.error_code()))
          .append(", gRPC message: ")
          .append(invocation_context->status.error_message())
          .append(", code: ")
          .append(std::to_string(invocation_context->response.common().status().code()))
          .append(", remark: ")
          .append(invocation_context->response.common().DebugString());
      MQException e(msg, FAILED_TO_SEND_MESSAGE, __FILE__, __LINE__);
      if (State::STARTED == state_.load(std::memory_order_relaxed)) {
        cb->onException(e);
      } else {
        SPDLOG_WARN("Client instance has stopped, state={}. Ignore exception raised while sending message: {}",
                    state_.load(std::memory_order_relaxed), e.what());
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
                                     const std::function<void(bool, const TopicRouteDataPtr&)>& cb) {

  RpcClientSharedPtr client = getRpcClient(target_host, false);
  if (!client) {
    SPDLOG_WARN("Failed to create RPC client for name server[host={}]", target_host);
    cb(false, nullptr);
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
      cb(false, nullptr);
      return;
    }

    if (google::rpc::Code::OK != invocation_context->response.common().status().code()) {
      SPDLOG_WARN("Server[host={}] failed to process query route request. Reason: {}",
                  invocation_context->remote_address, invocation_context->response.common().DebugString());
      cb(false, nullptr);
      return;
    }

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
    cb(true, ptr);
  };
  invocation_context->callback = callback;
  client->asyncQueryRoute(request, invocation_context);
}

void ClientManagerImpl::queryAssignment(const std::string& target, const Metadata& metadata,
                                        const QueryAssignmentRequest& request, std::chrono::milliseconds timeout,
                                        const std::function<void(bool, const QueryAssignmentResponse&)>& cb) {
  SPDLOG_DEBUG("Prepare to send query assignment request to broker[address={}]", target);
  std::shared_ptr<RpcClient> client = getRpcClient(target);

  auto callback = [&, cb](const InvocationContext<QueryAssignmentResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to query assignment. Reason: {}", invocation_context->status.error_message());
      cb(false, invocation_context->response);
      return;
    }

    if (google::rpc::Code::OK != invocation_context->response.common().status().code()) {
      SPDLOG_WARN("Server[host={}] failed to process query assignment request. Reason: {}",
                  invocation_context->remote_address, invocation_context->response.common().DebugString());
      cb(false, invocation_context->response);
      return;
    }

    cb(true, invocation_context->response);
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
      ReceiveMessageResult receive_result;
      this->processPopResult(invocation_context->context, invocation_context->response, receive_result,
                             invocation_context->remote_address);
      cb->onSuccess(receive_result);
    } else {
      SPDLOG_WARN("Failed to pop messages through GRPC from {}, gRPC code: {}, gRPC error message: {}",
                  invocation_context->remote_address, invocation_context->status.error_code(),
                  invocation_context->status.error_message());
      MQException e(invocation_context->status.error_message(), FAILED_TO_POP_MESSAGE_ASYNCHRONOUSLY, __FILE__,
                    __LINE__);
      cb->onException(e);
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
  case google::rpc::Code::OK:
    status = ReceiveMessageStatus::OK;
    break;
  case google::rpc::Code::RESOURCE_EXHAUSTED:
    status = ReceiveMessageStatus::RESOURCE_EXHAUSTED;
    SPDLOG_WARN("Too many pop requests in broker. Long polling is full in the broker side. BrokerAddress={}",
                target_host);
    break;
  case google::rpc::Code::DEADLINE_EXCEEDED:
    status = ReceiveMessageStatus::DEADLINE_EXCEEDED;
    break;
  case google::rpc::Code::NOT_FOUND:
    status = ReceiveMessageStatus::NOT_FOUND;
    break;
  default:
    SPDLOG_WARN("Pop response indicates server-side error. BrokerAddress={}, Reason={}", target_host,
                response.common().DebugString());
    status = ReceiveMessageStatus::INTERNAL;
    break;
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
  } break;

  case google::rpc::Code::DEADLINE_EXCEEDED:
    result.status_ = ReceiveMessageStatus::DEADLINE_EXCEEDED;
    break;

  case google::rpc::Code::RESOURCE_EXHAUSTED:
    result.status_ = ReceiveMessageStatus::RESOURCE_EXHAUSTED;
    break;

  case google::rpc::Code::OUT_OF_RANGE:
    result.status_ = ReceiveMessageStatus::OUT_OF_RANGE;
    result.next_offset_ = response.next_offset();
    break;
  }
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

Scheduler& ClientManagerImpl::getScheduler() { return scheduler_; }

void ClientManagerImpl::ack(const std::string& target, const Metadata& metadata, const AckMessageRequest& request,
                            std::chrono::milliseconds timeout, const std::function<void(bool)>& cb) {
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
    if (invocation_context->status.ok() &&
        google::rpc::Code::OK == invocation_context->response.common().status().code()) {
      cb(true);
    } else {
      cb(false);
    }
  };
  invocation_context->callback = callback;
  client->asyncAck(request, invocation_context);
}

void ClientManagerImpl::nack(const std::string& target_host, const Metadata& metadata,
                             const NackMessageRequest& request, std::chrono::milliseconds timeout,
                             const std::function<void(bool)>& completion_callback) {
  RpcClientSharedPtr client = getRpcClient(target_host);
  assert(client);
  auto invocation_context = new InvocationContext<NackMessageResponse>();
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);

  for (const auto& item : metadata) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  auto callback = [completion_callback](const InvocationContext<NackMessageResponse>* invocation_context) {
    if (invocation_context->status.ok() &&
        google::rpc::Code::OK == invocation_context->response.common().status().code()) {
      completion_callback(true);
    } else {
      completion_callback(false);
    }
  };
  invocation_context->callback = callback;
  client->asyncNack(request, invocation_context);
}

void ClientManagerImpl::endTransaction(const std::string& target_host, const Metadata& metadata,
                                       const EndTransactionRequest& request, std::chrono::milliseconds timeout,
                                       const std::function<void(bool, const EndTransactionResponse&)>& cb) {
  RpcClientSharedPtr client = getRpcClient(target_host);
  if (!client) {
    SPDLOG_WARN("No RPC client for {}", target_host);
    EndTransactionResponse response;
    cb(false, response);
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
    if (!invocation_context->status.ok() ||
        google::rpc::Code::OK != invocation_context->response.common().status().code()) {
      SPDLOG_WARN("Failed to endTransaction. TargetHost={}, gRPC statusCode={}, errorMessage={}", target_host.data(),
                  invocation_context->status.error_message(), invocation_context->status.error_message());
      cb(false, invocation_context->response);
      return;
    }

    SPDLOG_DEBUG("endTransaction completed OK. Response: {}", invocation_context->response.DebugString());
    cb(true, invocation_context->response);
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
                                    const std::function<void(bool, const QueryOffsetResponse&)>& cb) {
  auto client = getRpcClient(target_host);
  if (!client) {
    SPDLOG_WARN("Failed to get/create RPC client for {}", target_host);
    return;
  }

  auto invocation_context = new InvocationContext<QueryOffsetResponse>();
  invocation_context->remote_address = target_host;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + timeout);
  auto callback = [cb](const InvocationContext<QueryOffsetResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_WARN("Failed to send query offset request to {}. Reason: {}", invocation_context->remote_address,
                  invocation_context->status.error_message());
      cb(false, invocation_context->response);
      return;
    }

    if (google::rpc::Code::OK != invocation_context->response.common().status().code()) {
      SPDLOG_WARN("Server[host={}] failed to process query offset request. Reason: {}",
                  invocation_context->remote_address, invocation_context->response.common().DebugString());
      cb(false, invocation_context->response);
    }

    SPDLOG_DEBUG("Query offset from server[host={}] OK", invocation_context->remote_address);
    cb(true, invocation_context->response);
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