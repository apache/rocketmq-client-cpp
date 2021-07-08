#include "ClientInstance.h"

#include <chrono>
#include <utility>

#include "CRC.h"
#include "InvocationContext.h"
#include "LogInterceptor.h"
#include "LogInterceptorFactory.h"
#include "LoggerImpl.h"
#include "MessageAccessor.h"
#include "MessageSystemFlag.h"
#include "Metadata.h"
#include "MixAll.h"
#include "Partition.h"
#include "Protocol.h"
#include "RpcClientImpl.h"
#include "TlsHelper.h"
#include "UtilAll.h"
#include "grpcpp/create_channel.h"
#include "rocketmq/ErrorCode.h"
#include "rocketmq/MQMessageExt.h"

#ifdef ENABLE_TRACING
#include "TracingUtility.h"
#endif

ROCKETMQ_NAMESPACE_BEGIN

ClientInstance::ClientInstance(std::string arn)
    : arn_(std::move(arn)), state_(State::CREATED), completion_queue_(std::make_shared<CompletionQueue>()),
      callback_thread_pool_(std::make_shared<ThreadPool>(std::thread::hardware_concurrency())),
      latency_histogram_("Message-Latency", 11) {
  spdlog::set_level(spdlog::level::trace);

  inactive_rpc_client_detector_ = std::bind(&ClientInstance::doHealthCheck, this);
  inactive_rpc_client_detector_function_ = std::make_shared<Functional>(&inactive_rpc_client_detector_);

  heartbeat_loop_ = std::bind(&ClientInstance::doHeartbeat, this);
  heartbeat_loop_function_ = std::make_shared<Functional>(&heartbeat_loop_);

  assignLabels(latency_histogram_);

  stats_functor_ = std::bind(&ClientInstance::logStats, this);
  stats_function_ = std::make_shared<Functional>(&stats_functor_);

  server_authorization_check_config_ = std::make_shared<grpc::experimental::TlsServerAuthorizationCheckConfig>(
      std::make_shared<TlsServerAuthorizationChecker>());

  // Make use of encryption only at the moment.
  std::vector<grpc::experimental::IdentityKeyCertPair> identity_key_cert_list;
  grpc::experimental::IdentityKeyCertPair pair{.private_key = TlsHelper::client_private_key,
                                               .certificate_chain = TlsHelper::client_certificate_chain};
  identity_key_cert_list.emplace_back(pair);
  certificate_provider_ =
      std::make_shared<grpc::experimental::StaticDataCertificateProvider>(TlsHelper::CA, identity_key_cert_list);
  tls_channel_credential_options_.set_server_verification_option(GRPC_TLS_SKIP_ALL_SERVER_VERIFICATION);
  tls_channel_credential_options_.set_certificate_provider(certificate_provider_);
  tls_channel_credential_options_.set_server_authorization_check_config(server_authorization_check_config_);
  tls_channel_credential_options_.watch_root_certs();
  tls_channel_credential_options_.watch_identity_key_cert_pairs();
  channel_credential_ = grpc::experimental::TlsCredentials(tls_channel_credential_options_);
  SPDLOG_INFO("ClientInstance[ARN={}] created", arn_);
}

ClientInstance::~ClientInstance() {
  shutdown();
  SPDLOG_INFO("ClientInstance[ARN={}] destructed", arn_);
}

void ClientInstance::start() {
  if (State::CREATED != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state: {}", state_.load(std::memory_order_relaxed));
    return;
  }
  state_.store(State::STARTING, std::memory_order_relaxed);

  scheduler_.start();

  bool scheduled =
      scheduler_.schedule(inactive_rpc_client_detector_function_, std::chrono::seconds(5), std::chrono::seconds(15));
  if (scheduled) {
    SPDLOG_INFO("inactive client detector scheduled");
  } else {
    SPDLOG_ERROR("Failed to schedule inactive client detector");
  }

  scheduled = scheduler_.schedule(heartbeat_loop_function_, std::chrono::seconds(1), std::chrono::seconds(10));
  if (scheduled) {
    SPDLOG_INFO("Heartbeat loop scheduled");
  } else {
    SPDLOG_ERROR("Failed to schedule consumer heartbeat loop");
  }

  completion_queue_thread_ = std::thread(std::bind(&ClientInstance::pollCompletionQueue, this));

  scheduled = scheduler_.schedule(stats_function_, std::chrono::seconds(0), std::chrono::seconds(10));
  if (scheduled) {
    SPDLOG_INFO("logStats loop scheduled");
  } else {
    SPDLOG_ERROR("Failed to schedule logStats loop");
  }

  state_.store(State::STARTED, std::memory_order_relaxed);
}

void ClientInstance::shutdown() {
  SPDLOG_DEBUG("Client instance shutdown");
  if (State::STARTED != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state: {}", state_.load(std::memory_order_relaxed));
    return;
  }

  state_.store(STOPPING, std::memory_order_relaxed);
  {
    absl::MutexLock lk(&inactive_rpc_clients_mtx_);
    inactive_rpc_clients_.clear();
    SPDLOG_DEBUG("CompletionQueue of inactive clients stopped");
  }

  {
    absl::MutexLock lk(&rpc_clients_mtx_);
    rpc_clients_.clear();
    SPDLOG_DEBUG("CompletionQueue of active clients stopped");
  }

  scheduler_.shutdown();

  completion_queue_->Shutdown();
  if (completion_queue_thread_.joinable()) {
    completion_queue_thread_.join();
  }
  SPDLOG_DEBUG("Completion queue thread completes OK");

  state_.store(State::STOPPED, std::memory_order_relaxed);
  SPDLOG_DEBUG("Client instance stopped");
}

void ClientInstance::assignLabels(Histogram& histogram) {
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

bool ClientInstance::addInactiveRpcClient(const std::string& target_host) {
  std::shared_ptr<RpcClient> client;
  // get the inactive rpc client and remove from rpc_clients_(active client)
  {
    absl::MutexLock lock(&rpc_clients_mtx_);
    if (!rpc_clients_.contains(target_host)) {
      SPDLOG_ERROR("RpcClient {} can not be found in rpc_clients_, remove error", target_host);
      return false;
    } else {
      client = rpc_clients_[target_host];
      rpc_clients_.erase(target_host);
    }
  }
  // add to inactive_rpc_clients_
  {
    absl::MutexLock lock(&inactive_rpc_clients_mtx_);
    inactive_rpc_clients_.insert(std::make_pair(target_host, client));
  }
  return true;
}

bool ClientInstance::removeInactiveRpcClient(const std::string& target_host) {
  std::shared_ptr<RpcClient> client;
  {
    absl::MutexLock lock(&inactive_rpc_clients_mtx_);
    if (!inactive_rpc_clients_.contains(target_host)) {
      SPDLOG_ERROR("RpcClient {} can not be found in inactive_rpc_clients_, remove error", target_host);
      return false;
    } else {
      client = inactive_rpc_clients_[target_host];
      inactive_rpc_clients_.erase(target_host);
    }
  }

  {
    absl::MutexLock lock(&rpc_clients_mtx_);
    rpc_clients_.insert(std::make_pair(target_host, client));
  }
  SPDLOG_DEBUG("Migrated {} from inactive to active list", target_host);
  return true;
}

bool ClientInstance::getInactiveRpcClient(absl::string_view target_host, RpcClientSharedPtr& client) {
  std::string target(target_host.data(), target_host.length());
  {
    absl::MutexLock lock(&inactive_rpc_clients_mtx_);
    if (!inactive_rpc_clients_.contains(target)) {
      SPDLOG_DEBUG("RpcClient {} can not be found in inactive_rpc_clients_. Host:{}", target);
      return false;
    } else {
      client = inactive_rpc_clients_[target];
    }
  }
  return true;
}

void ClientInstance::doHealthCheck() {
  SPDLOG_DEBUG("Start to perform health check for inactive clients");
  if (State::STARTED != state_.load(std::memory_order_relaxed) &&
      State::STARTING != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state={}.", state_.load(std::memory_order_relaxed));
    return;
  }
  cleanOfflineRpcClients();
  clientHealthCheck();
  SPDLOG_DEBUG("Health check completed");
}

/**
 * TODO:(lingchu) optimize implementation. Client may just select one broker by random.
 */
#ifdef ENABLE_TRACING
void ClientInstance::updateTraceProvider() {
  SPDLOG_DEBUG("Start to update global trace provider");
  if (State::STARTED != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state={}.", state_.load(std::memory_order_relaxed));
    return;
  }
  absl::flat_hash_set<std::string> exporter_endpoint_set;
  {
    absl::MutexLock lock(&topic_route_table_mtx_);
    for (const auto& route_entry : topic_route_table_) {
      for (const auto& partition : route_entry.second->partitions()) {
        const std::string& broker_name = partition.broker().name();
        if (MixAll::MASTER_BROKER_ID != partition.broker().id()) {
          continue;
        }
        exporter_endpoint_set.insert(partition.broker().serviceAddress());
      }
    }
  }
  {
    absl::MutexLock lock(&exporter_endpoint_set_mtx_);
    // Available endpoint was not changed, no need to change global trace provider.
    if (exporter_endpoint_set == exporter_endpoint_set_) {
      return;
    }

    // TODO: support ipv6.
    std::string exporter_endpoint;
    if (!exporter_endpoint_set.empty()) {
      exporter_endpoint = absl::StrJoin(exporter_endpoint_set.begin(), exporter_endpoint_set.end(), ",");
    }

    opentelemetry::exporter::otlp::OtlpExporterOptions exporter_options;
    // If no available export, use default export here.
    if (!exporter_endpoint_set.empty()) {
      exporter_options.endpoint = exporter_endpoint;
    }

    auto exporter = std::unique_ptr<sdktrace::SpanExporter>(new otlp::OtlpExporter(exporter_options));

    sdktrace::BatchSpanProcessorOptions options{};
    options.max_queue_size = 32768;
    options.max_export_batch_size = 16384;
    options.schedule_delay_millis = std::chrono::milliseconds(1000);

    auto processor =
        std::shared_ptr<sdktrace::SpanProcessor>(new sdktrace::BatchSpanProcessor(std::move(exporter), options));
    auto provider = nostd::shared_ptr<trace::TracerProvider>(new sdktrace::TracerProvider(processor));
    // Set the global trace provider
    trace::Provider::SetTracerProvider(provider);

    exporter_endpoint_set_.clear();
    exporter_endpoint_set_.merge(exporter_endpoint_set);
  }
}

nostd::shared_ptr<trace::Tracer> ClientInstance::getTracer() {
  {
    absl::MutexLock lock(&exporter_endpoint_set_mtx_);
    if (exporter_endpoint_set_.empty()) {
      return nostd::shared_ptr<trace::Tracer>(nullptr);
    }
    auto provider = trace::Provider::GetTracerProvider();
    return provider->GetTracer("RocketmqClient");
  }
}

#endif

void ClientInstance::cleanOfflineRpcClients() {
  absl::flat_hash_set<std::string> hosts;
  {
    absl::MutexLock lk(&clients_mtx_);
    for (const auto& item : clients_) {
      std::shared_ptr<ClientCallback> client = item.lock();
      if (!client) {
        continue;
      }
      client->activeHosts(hosts);
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

  {
    absl::MutexLock lock(&inactive_rpc_clients_mtx_);
    for (auto it = inactive_rpc_clients_.begin(); it != inactive_rpc_clients_.end();) {
      std::string host = it->first;
      if (it->second->needHeartbeat() && !hosts.contains(host)) {
        inactive_rpc_clients_.erase(it++);
        SPDLOG_INFO("Removed RPC client whose peer is offline. RemoteHost={}", host);
      } else {
        it++;
      }
    }
  }
}

bool ClientInstance::isRpcClientActive(absl::string_view target_host) {
  absl::MutexLock lock(&inactive_rpc_clients_mtx_);
  return !inactive_rpc_clients_.contains(target_host);
}

void ClientInstance::getAllInactiveRpcClientHost(absl::flat_hash_set<std::string>& client_hosts) {
  client_hosts.clear();
  {
    absl::MutexLock lock(&inactive_rpc_clients_mtx_);
    for (const auto& entry : inactive_rpc_clients_) {
      client_hosts.insert(entry.first);
    }
  }
}

void ClientInstance::clientHealthCheck() {
  absl::flat_hash_set<std::string> clientsHosts;
  getAllInactiveRpcClientHost(clientsHosts);
  for (const auto& clientHost : clientsHosts) {
    RpcClientSharedPtr client;
    if (!getInactiveRpcClient(clientHost, client)) {
      SPDLOG_ERROR("get inactive client error, client Host :{}", clientHost);
    }
    SPDLOG_DEBUG("check client(host: {}) health status", clientHost);
    // detect if the rpc client is healthy
    HealthCheckRequest request;
    HealthCheckResponse response;
    request.set_client_host(clientHost);
    auto invocation_context = new InvocationContext<HealthCheckResponse>();

    // TODO: Acquire metadata from ProducerImpl/ConsumerImpl.
    absl::flat_hash_map<std::string, std::string> metadata;

    // TODO: Sign metadata

    for (const auto& item : metadata) {
      invocation_context->context_.AddMetadata(item.first, item.second);
    }

    auto callback = [this, clientHost](const grpc::Status& status, const grpc::ClientContext& context,
                                       const HealthCheckResponse& response) {
      if (status.ok() && response.common().status().code() == google::rpc::Code::OK) {
        SPDLOG_INFO("Inactive client {} passes health check, thus, is considered active", clientHost);
        if (!removeInactiveRpcClient(clientHost)) {
          SPDLOG_ERROR("Failed to move the previous inactive client to active list", clientHost);
        }
      } else {
        SPDLOG_INFO("client for host={} failed to pass health check again. Reason: {}", clientHost,
                    response.common().DebugString());
      }
    };
    invocation_context->callback_ = callback;

    client->asyncHealthCheck(request, invocation_context);
  } // end for
}

void ClientInstance::heartbeat(const std::string& target_host,
                               const absl::flat_hash_map<std::string, std::string>& metadata,
                               const HeartbeatRequest& request, std::chrono::milliseconds timeout,
                               const std::function<void(bool, const HeartbeatResponse&)>& cb) {
  auto client = getRpcClient(target_host, true);
  if (!client) {
    return;
  }

  auto invocation_context = new InvocationContext<HeartbeatResponse>();
  for (const auto& item : metadata) {
    invocation_context->context_.AddMetadata(item.first, item.second);
  }

  auto callback = [target_host, cb](const grpc::Status& status, const grpc::ClientContext& context,
                                    const HeartbeatResponse& response) {
    if (status.ok()) {
      if (google::rpc::Code::OK == response.common().status().code()) {
        SPDLOG_DEBUG("Send heartbeat to target_host={}, gRPC status OK", target_host);
        cb(true, response);
      } else {
        SPDLOG_WARN("Server[{}] failed to process heartbeat. Reason: {}", target_host, response.common().DebugString());
        cb(false, response);
      }
    } else {
      SPDLOG_WARN("Failed to send heartbeat to target_host={}. GRPC code: {}, message : {}", target_host,
                  status.error_code(), status.error_message());
      cb(false, response);
    }
  };
  invocation_context->callback_ = callback;
  invocation_context->context_.set_deadline(std::chrono::system_clock::now() + timeout);
  client->asyncHeartbeat(request, invocation_context);
}

void ClientInstance::doHeartbeat() {
  if (State::STARTED != state_.load(std::memory_order_relaxed) &&
      State::STARTING != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state={}.", state_.load(std::memory_order_relaxed));
    return;
  }

  {
    absl::MutexLock lk(&clients_mtx_);
    for (const auto& item : clients_) {
      auto client = item.lock();
      if (client) {
        client->heartbeat();
      }
    }
  }
}

void ClientInstance::pollCompletionQueue() {

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
      callback_thread_pool_->enqueue(callback);
    }
    SPDLOG_INFO("CompletionQueue is fully drained and shut down");
  }
  SPDLOG_INFO("pollCompletionQueue completed and quit");
}

bool ClientInstance::send(const std::string& target, const absl::flat_hash_map<std::string, std::string>& metadata,
                          SendMessageRequest& request, SendCallback* callback) {
  assert(callback);
  std::string target_host(target.data(), target.length());
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
  for (const auto& entry : metadata) {
    invocation_context->context_.AddMetadata(entry.first, entry.second);
  }

  const std::string& topic = request.message().topic().name();
  auto completion_callback = [topic, callback, target_host, span, this](const grpc::Status& status,
                                                                        const grpc::ClientContext&,
                                                                        const SendMessageResponse& response) {
    if (status.ok() && google::rpc::Code::OK == response.common().status().code()) {
#ifdef ENABLE_TRACING
      if (span) {
        span->SetAttribute(TracingUtility::get().success_, true);
        span->End();
      }
#endif
      SendResult send_result;
      send_result.setMsgId(response.message_id());
      send_result.setQueueOffset(-1);
      MQMessageQueue message_queue;
      message_queue.setQueueId(-1);
      message_queue.setTopic(topic);
      send_result.setMessageQueue(message_queue);
      if (State::STARTED == state_.load(std::memory_order_relaxed)) {
        callback->onSuccess(send_result);
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

      if (!status.ok()) {
        SPDLOG_WARN("Failed to send message to {} due to gRPC error. gRPC code: {}, gRPC error message: {}",
                    target_host, status.error_code(), status.error_message());
      }
      std::string msg;
      msg.append("gRPC code: ")
          .append(std::to_string(status.error_code()))
          .append(", gRPC message: ")
          .append(status.error_message())
          .append(", code: ")
          .append(std::to_string(response.common().status().code()))
          .append(", remark: ")
          .append(response.common().DebugString());
      MQException e(msg, FAILED_TO_SEND_MESSAGE, __FILE__, __LINE__);
      if (State::STARTED == state_.load(std::memory_order_relaxed)) {
        callback->onException(e);
      } else {
        SPDLOG_WARN("Client instance has stopped, state={}. Ignore exception raised while sending message: {}",
                    state_.load(std::memory_order_relaxed), e.what());
      }
    }
  };

  invocation_context->callback_ = completion_callback;
  client->asyncSend(request, invocation_context);
  return true;
}

RpcClientSharedPtr ClientInstance::getRpcClient(const std::string& target_host, bool need_heartbeat) {
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

  return client;
}

void ClientInstance::addRpcClient(const std::string& target_host, const RpcClientSharedPtr& client) {
  {
    absl::MutexLock lock(&rpc_clients_mtx_);
    rpc_clients_.insert_or_assign(target_host, client);
  }
}

SendResult ClientInstance::processSendResponse(const MQMessageQueue& message_queue,
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

void ClientInstance::addClientObserver(std::weak_ptr<ClientCallback> client) {
  absl::MutexLock lk(&clients_mtx_);
  clients_.emplace_back(std::move(client));
}

void ClientInstance::resolveRoute(const std::string& target_host,
                                  const absl::flat_hash_map<std::string, std::string>& metadata,
                                  const QueryRouteRequest& request, std::chrono::milliseconds timeout,
                                  const std::function<void(bool, const TopicRouteDataPtr&)>& cb) {

  RpcClientSharedPtr client = getRpcClient(target_host, false);
  if (!client) {
    SPDLOG_WARN("Failed to create RPC client for name server[host={}]", target_host);
    cb(false, nullptr);
    return;
  }

  auto invocation_context = new InvocationContext<QueryRouteResponse>();
  invocation_context->context_.set_deadline(std::chrono::system_clock::now() + timeout);
  for (const auto& item : metadata) {
    invocation_context->context_.AddMetadata(item.first, item.second);
  }

  auto callback = [target_host, cb](const grpc::Status& status, const grpc::ClientContext& client_context,
                                    const QueryRouteResponse& response) {
    if (!status.ok()) {
      SPDLOG_WARN("Failed to send query route request to server[host={}]. Reason: {}", target_host,
                  status.error_message());
      cb(false, nullptr);
      return;
    }

    if (google::rpc::Code::OK != response.common().status().code()) {
      SPDLOG_WARN("Server[host={}] failed to process query route request. Reason: {}", target_host,
                  response.common().DebugString());
      cb(false, nullptr);
      return;
    }

    auto& partitions = response.partitions();

    std::vector<Partition> topic_partitions;
    for (const auto& partition : partitions) {
      Topic t(partition.topic().arn(), partition.topic().name());

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

    auto ptr = std::make_shared<TopicRouteData>(std::move(topic_partitions), response.DebugString());
    cb(true, ptr);
  };
  invocation_context->callback_ = callback;
  client->asyncQueryRoute(request, invocation_context);
}

void ClientInstance::queryAssignment(const std::string& target,
                                     const absl::flat_hash_map<std::string, std::string>& metadata,
                                     const QueryAssignmentRequest& request, std::chrono::milliseconds timeout,
                                     const std::function<void(bool, const QueryAssignmentResponse&)>& cb) {
  SPDLOG_DEBUG("Prepare to send query assignment request to broker[address={}]", target);
  std::shared_ptr<RpcClient> client = getRpcClient(target);

  auto callback = [&, target, cb](const grpc::Status& status, const grpc::ClientContext& context,
                                  const QueryAssignmentResponse& response) {
    if (!status.ok()) {
      SPDLOG_WARN("Failed to query assignment. Reason: {}", status.error_message());
      cb(false, response);
      return;
    }

    if (google::rpc::Code::OK != response.common().status().code()) {
      SPDLOG_WARN("Server[host={}] failed to process query assignment request. Reason: {}", target,
                  response.common().DebugString());
      cb(false, response);
      return;
    }

    cb(true, response);
  };

  auto invocation_context = new InvocationContext<QueryAssignmentResponse>();
  for (const auto& item : metadata) {
    invocation_context->context_.AddMetadata(item.first, item.second);
  }
  invocation_context->context_.set_deadline(std::chrono::system_clock::now() + timeout);
  invocation_context->callback_ = callback;
  client->asyncQueryAssignment(request, invocation_context);
}

void ClientInstance::receiveMessage(absl::string_view target,
                                    const absl::flat_hash_map<std::string, std::string>& metadata,
                                    const ReceiveMessageRequest& request, std::chrono::milliseconds timeout,
                                    std::shared_ptr<ReceiveMessageCallback>& cb) {
  std::string target_host(target.data(), target.length());
  SPDLOG_DEBUG("Prepare to pop message from {} asynchronously. Request: {}", target_host, request.DebugString());
  RpcClientSharedPtr client = getRpcClient(target_host);

  auto invocation_context = new InvocationContext<ReceiveMessageResponse>();

  if (!metadata.empty()) {
    for (const auto& item : metadata) {
      invocation_context->context_.AddMetadata(item.first, item.second);
    }
  }
  invocation_context->context_.set_deadline(std::chrono::system_clock::now() + timeout);

  auto callback = [this, cb, target_host](const grpc::Status& status, const grpc::ClientContext& client_context,
                                          const ReceiveMessageResponse& response) {
    if (status.ok()) {
      SPDLOG_DEBUG("Received pop response through gRPC from brokerAddress={}", target_host);
      ReceiveMessageResult receive_result;
      this->processPopResult(client_context, response, receive_result, target_host);
      cb->onSuccess(receive_result);
    } else {
      SPDLOG_WARN("Failed to pop messages through GRPC from {}, gRPC code: {}, gRPC error message: {}", target_host,
                  status.error_code(), status.error_message());
      MQException e(status.error_message(), FAILED_TO_POP_MESSAGE_ASYNCHRONOUSLY, __FILE__, __LINE__);
      cb->onException(e);
    }
  };
  invocation_context->callback_ = callback;
  client->asyncReceive(request, invocation_context);
}

void ClientInstance::pullMessage(absl::string_view target, absl::flat_hash_map<std::string, std::string>& metadata,
                                 const PullMessageRequest& request,
                                 std::shared_ptr<ReceiveMessageCallback>& receive_callback) {
  std::string target_host(target.data(), target.length());
  SPDLOG_DEBUG("Prepare to pull message from {} asynchronously", target_host);
  RpcClientSharedPtr client = getRpcClient(target_host);
  auto invocation_context = new InvocationContext<PullMessageResponse>();
  if (!metadata.empty()) {
    for (const auto& entry : metadata) {
      invocation_context->context_.AddMetadata(entry.first, entry.second);
    }
  }

  auto callback = [this, receive_callback, target_host](const grpc::Status& status,
                                                        const grpc::ClientContext& client_context,
                                                        const PullMessageResponse& response) {
    if (status.ok()) {
      SPDLOG_DEBUG("Received message response through gRPC from brokerAddress={}", target_host);
      ReceiveMessageResult receive_result;
      this->processPullResult(client_context, response, receive_result, target_host);
      receive_callback->onSuccess(receive_result);
    } else {
      SPDLOG_WARN("Failed to pull messages through GRPC from {}, gRPC code: {}, gRPC error message: {}", target_host,
                  status.error_code(), status.error_message());
      MQException e(status.error_message(), FAILED_TO_POP_MESSAGE_ASYNCHRONOUSLY, __FILE__, __LINE__);
      receive_callback->onException(e);
    }
  };
  invocation_context->callback_ = callback;
  client->asyncPull(request, invocation_context);
}

void ClientInstance::processPopResult(const grpc::ClientContext& client_context, const ReceiveMessageResponse& response,
                                      ReceiveMessageResult& result, absl::string_view target_host) {
  // process response to result
  ReceiveMessageStatus status;
  std::string broker_address = std::string(target_host.data(), target_host.length());
  switch (response.common().status().code()) {
  case google::rpc::Code::OK:
    status = ReceiveMessageStatus::OK;
    break;
  case google::rpc::Code::RESOURCE_EXHAUSTED:
    status = ReceiveMessageStatus::RESOURCE_EXHAUSTED;
    SPDLOG_WARN("Too many pop requests in broker. Long polling is full in the broker side. BrokerAddress={}",
                broker_address);
    break;
  case google::rpc::Code::DEADLINE_EXCEEDED:
    status = ReceiveMessageStatus::DEADLINE_EXCEEDED;
    break;
  case google::rpc::Code::NOT_FOUND:
    status = ReceiveMessageStatus::NOT_FOUND;
    break;
  default:
    SPDLOG_WARN("Pop response indicates server-side error. BrokerAddress={}, Reason={}", broker_address,
                response.common().DebugString());
    status = ReceiveMessageStatus::INTERNAL;
    break;
  }

  const auto& metadata = client_context.GetServerInitialMetadata();
  auto search = metadata.find(Metadata::REQUEST_ID_KEY);
  if (metadata.end() == search) {
    SPDLOG_WARN("Expected header {} is missing", Metadata::AUTHORIZATION);
  } else {
    result.requestId(std::string(search->second.data(), search->second.length()));
  }

  result.sourceHost(broker_address);

  std::vector<MQMessageExt> msg_found_list;
  if (ReceiveMessageStatus::OK == status) {
    for (auto& item : response.messages()) {
      MQMessageExt message_ext;
      if (wrapMessage(item, message_ext)) {
        MessageAccessor::setTargetEndpoint(message_ext, broker_address);
        msg_found_list.emplace_back(message_ext);
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

void ClientInstance::processPullResult(const grpc::ClientContext& client_context, const PullMessageResponse& response,
                                       ReceiveMessageResult& result, absl::string_view target_host) {
  if (google::rpc::Code::OK != response.common().status().code()) {
    result.status_ = ReceiveMessageStatus::INTERNAL;
    return;
  }
  result.sourceHost(std::string(target_host.data(), target_host.length()));
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

bool ClientInstance::wrapMessage(const rmq::Message& item, MQMessageExt& message_ext) {
  assert(item.topic().arn() == arn_);

  // base
  message_ext.setTopic(item.topic().name());

  const auto& system_attributes = item.system_attribute();

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
      std::uint32_t crc = CRC::Calculate(item.body().data(), item.body().length(), CRC::CRC_32());
      SPDLOG_DEBUG("Compute and compare CRC32 checksum. Actual: {}, expect: {}", crc, digest.checksum());
      body_digest_match = (std::stoul(digest.checksum()) == crc);
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
  case rmq::Encoding::SNAPPY: {
    SPDLOG_WARN("Snappy encoding is not supported yet");
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

  timeval tv{.tv_sec = 0, .tv_usec = 0};

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

  // Receipt-handle
  MessageAccessor::setReceiptHandle(message_ext, system_attributes.receipt_handle());

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

  // Delivery Count
  MessageAccessor::setDeliveryCount(message_ext, system_attributes.delivery_count());

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

Scheduler& ClientInstance::getScheduler() { return scheduler_; }

void ClientInstance::ack(const std::string& target, const absl::flat_hash_map<std::string, std::string>& metadata,
                         const AckMessageRequest& request, std::chrono::milliseconds timeout,
                         const std::function<void(bool)>& completion_callback) {
  std::string target_host(target.data(), target.length());
  SPDLOG_DEBUG("Prepare to ack message against {} asynchronously. AckMessageRequest: {}", target_host,
               request.DebugString());
  RpcClientSharedPtr client = getRpcClient(target_host);

  auto invocation_context = new InvocationContext<AckMessageResponse>();
  invocation_context->context_.set_deadline(std::chrono::system_clock::now() + timeout);

  for (const auto& item : metadata) {
    invocation_context->context_.AddMetadata(item.first, item.second);
  }

  // TODO: Use capture by move and pass-by-value paradigm when C++ 14 is available.
  auto callback = [target_host, request, completion_callback](const grpc::Status& status,
                                                              const grpc::ClientContext& client_context,
                                                              const AckMessageResponse& response) {
    if (status.ok() && google::rpc::Code::OK == response.common().status().code()) {
      completion_callback(true);
    } else {
      completion_callback(false);
    }
  };
  invocation_context->callback_ = callback;
  client->asyncAck(request, invocation_context);
}

void ClientInstance::nack(const std::string& target_host, const absl::flat_hash_map<std::string, std::string>& metadata,
                          const NackMessageRequest& request, std::chrono::milliseconds timeout,
                          const std::function<void(bool)>& completion_callback) {
  RpcClientSharedPtr client = getRpcClient(target_host);
  assert(client);
  auto invocation_context = new InvocationContext<NackMessageResponse>();

  invocation_context->context_.set_deadline(std::chrono::system_clock::now() + timeout);

  for (const auto& item : metadata) {
    invocation_context->context_.AddMetadata(item.first, item.second);
  }

  auto callback = [completion_callback](const grpc::Status& status, const grpc::ClientContext& context,
                                        const NackMessageResponse& response) {
    if (status.ok() && google::rpc::Code::OK == response.common().status().code()) {
      completion_callback(true);
    } else {
      completion_callback(false);
    }
  };
  invocation_context->callback_ = callback;
  client->asyncNack(request, invocation_context);
}

void ClientInstance::endTransaction(const std::string& target_host,
                                    const absl::flat_hash_map<std::string, std::string>& metadata,
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
  for (const auto& item : metadata) {
    invocation_context->context_.AddMetadata(item.first, item.second);
  }

  // Set RPC deadline.
  auto deadline = std::chrono::system_clock::now() + timeout;
  invocation_context->context_.set_deadline(deadline);

  auto callback = [target_host, cb](const grpc::Status& status, const grpc::ClientContext& context,
                                    const EndTransactionResponse& response) {
    if (!status.ok() || google::rpc::Code::OK != response.common().status().code()) {
      SPDLOG_WARN("Failed to endTransaction. TargetHost={}, gRPC statusCode={}, errorMessage={}", target_host.data(),
                  status.error_message(), status.error_message());
      cb(false, response);
      return;
    }

    SPDLOG_DEBUG("endTransaction completed OK. Response: {}", response.DebugString());
    cb(true, response);
  };
  invocation_context->callback_ = callback;
  client->asyncEndTransaction(request, invocation_context);
}

void ClientInstance::multiplexingCall(const std::string& target_host,
                                      const absl::flat_hash_map<std::string, std::string>& metadata,
                                      const MultiplexingRequest& request, std::chrono::milliseconds timeout,
                                      const std::function<void(bool, const MultiplexingResponse&)>& cb) {
  RpcClientSharedPtr client = getRpcClient(target_host);
  if (!client) {
    SPDLOG_WARN("No RPC client for {}", target_host);
    MultiplexingResponse response;
    cb(false, response);
    return;
  }

  SPDLOG_DEBUG("Prepare to endTransaction. TargetHost={}, Request: {}", target_host.data(), request.DebugString());

  auto invocation_context = new InvocationContext<MultiplexingResponse>();
  for (const auto& item : metadata) {
    invocation_context->context_.AddMetadata(item.first, item.second);
  }

  // Set RPC deadline.
  auto deadline = std::chrono::system_clock::now() + timeout;
  invocation_context->context_.set_deadline(deadline);

  auto callback = [target_host, cb](const grpc::Status& status, const grpc::ClientContext& context,
                                    const MultiplexingResponse& response) {
    if (!status.ok()) {
      SPDLOG_WARN("Failed to apply multiplexing-call. TargetHost={}, gRPC statusCode={}, errorMessage={}",
                  target_host.data(), status.error_message(), status.error_message());
      cb(false, response);
      return;
    }

    SPDLOG_DEBUG("endTransaction completed OK. Response: {}", response.DebugString());
    cb(true, response);
  };
  invocation_context->callback_ = callback;
  client->asyncMultiplexingCall(request, invocation_context);
}

void ClientInstance::pullMessage(const std::string& target_host,
                                 const absl::flat_hash_map<std::string, std::string>& metadata,
                                 const PullMessageRequest& request, std::chrono::milliseconds timeout,
                                 const std::function<void(bool, const PullMessageResponse&)>& cb) {
  auto client = getRpcClient(target_host);
  if (!client) {
    PullMessageResponse response;
    cb(false, response);
    return;
  }

  auto invocation_context = new InvocationContext<PullMessageResponse>();
  invocation_context->context_.set_deadline(std::chrono::system_clock::now() + timeout);
  for (const auto& item : metadata) {
    invocation_context->context_.AddMetadata(item.first, item.second);
  }

  auto callback = [target_host, cb](const grpc::Status& status, const grpc::ClientContext& client_context,
                                    const PullMessageResponse& response) {
    if (!status.ok()) {
      SPDLOG_WARN("Failed to send pullMessage request to {}", target_host);
      cb(false, response);
      return;
    }

    SPDLOG_DEBUG("Received pullMessage response from server[host={}]", target_host);
    cb(true, response);
  };
  invocation_context->callback_ = callback;

  client->asyncPull(request, invocation_context);
}

void ClientInstance::logStats() {
  std::string stats;
  latency_histogram_.reportAndReset(stats);
  SPDLOG_INFO("{}", stats);
}

ROCKETMQ_NAMESPACE_END