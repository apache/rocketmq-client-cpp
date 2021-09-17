#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <string>
#include <utility>

#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "apache/rocketmq/v1/definition.pb.h"

#include "ClientImpl.h"
#include "ClientManagerFactory.h"
#include "HttpClientImpl.h"
#include "InvocationContext.h"
#include "LoggerImpl.h"
#include "MessageAccessor.h"
#include "Signature.h"
#include "rocketmq/MQMessageExt.h"

ROCKETMQ_NAMESPACE_BEGIN

ClientImpl::ClientImpl(std::string group_name) : ClientConfigImpl(std::move(group_name)), state_(State::CREATED) {}

void ClientImpl::start() {
  State expected = CREATED;
  if (!state_.compare_exchange_strong(expected, State::STARTING)) {
    SPDLOG_WARN("Unexpected state: {}", state_.load(std::memory_order_relaxed));
    return;
  }

  if (!name_server_resolver_) {
    SPDLOG_ERROR("No name server resolver is configured.");
    abort();
  }
  name_server_resolver_->start();

  client_manager_ = ClientManagerFactory::getInstance().getClientManager(*this);
  client_manager_->start();

  exporter_ = std::make_shared<OtlpExporter>(client_manager_, this);
  exporter_->start();

  std::weak_ptr<ClientImpl> ptr(self());

  auto route_update_functor = [ptr]() {
    std::shared_ptr<ClientImpl> base = ptr.lock();
    if (base) {
      base->updateRouteInfo();
    }
  };

  route_update_handle_ = client_manager_->getScheduler().schedule(route_update_functor, UPDATE_ROUTE_TASK_NAME,
                                                                  std::chrono::seconds(10), std::chrono::seconds(30));
  state_.store(State::STARTED);
}

void ClientImpl::shutdown() {
  state_.store(State::STOPPING, std::memory_order_relaxed);

  name_server_resolver_->shutdown();

  if (route_update_handle_) {
    client_manager_->getScheduler().cancel(route_update_handle_);
  }

  notifyClientTermination();

  client_manager_.reset();
}

const char* ClientImpl::UPDATE_ROUTE_TASK_NAME = "route_updater";

void ClientImpl::endpointsInUse(absl::flat_hash_set<std::string>& endpoints) {
  absl::MutexLock lk(&topic_route_table_mtx_);
  for (const auto& item : topic_route_table_) {
    for (const auto& partition : item.second->partitions()) {
      std::string endpoint = partition.asMessageQueue().serviceAddress();
      if (!endpoints.contains(endpoint)) {
        endpoints.emplace(std::move(endpoint));
      }
    }
  }
}

void ClientImpl::getRouteFor(const std::string& topic, const std::function<void(TopicRouteDataPtr)>& cb) {
  TopicRouteDataPtr route = nullptr;
  {
    absl::MutexLock lock(&topic_route_table_mtx_);
    if (topic_route_table_.contains(topic)) {
      route = topic_route_table_.at(topic);
    }
  }

  if (route) {
    cb(route);
    return;
  }

  bool query_backend = true;
  {
    absl::MutexLock lk(&inflight_route_requests_mtx_);
    {
      absl::MutexLock route_table_lock(&topic_route_table_mtx_);
      if (topic_route_table_.contains(topic)) {
        route = topic_route_table_.at(topic);
        query_backend = false;
      }
    }

    if (query_backend) {
      if (inflight_route_requests_.contains(topic)) {
        inflight_route_requests_.at(topic).emplace_back(cb);
        SPDLOG_DEBUG("Would reuse prior route request for topic={}", topic);
        return;
      } else {
        std::vector<std::function<void(const TopicRouteDataPtr&)>> inflight{cb};
        inflight_route_requests_.insert({topic, inflight});
        SPDLOG_INFO("Create inflight route query cache for topic={}", topic);
      }
    }
  }

  if (!query_backend && route) {
    cb(route);
  } else {
    fetchRouteFor(topic, std::bind(&ClientImpl::onTopicRouteReady, this, topic, std::placeholders::_1));
  }
}

void ClientImpl::setAccessPoint(rmq::Endpoints* endpoints) {
  std::vector<std::pair<std::string, std::uint16_t>> pairs;
  {
    std::vector<std::string> name_server_list = name_server_resolver_->resolve();
    for (const auto& name_server_item : name_server_list) {
      std::string::size_type pos = name_server_item.rfind(':');
      if (std::string::npos == pos) {
        continue;
      }
      std::string host(name_server_item.substr(0, pos));
      std::string port(name_server_item.substr(pos + 1));
      pairs.emplace_back(std::make_pair(host, std::stoi(port)));
    }
  }

  if (!pairs.empty()) {
    for (const auto& host_port : pairs) {
      auto address = new rmq::Address();
      address->set_port(host_port.second);
      address->set_host(host_port.first);
      endpoints->mutable_addresses()->AddAllocated(address);
    }

    if (MixAll::isIPv4(pairs.begin()->first)) {
      endpoints->set_scheme(rmq::AddressScheme::IPv4);
    } else if (absl::StrContains(pairs.begin()->first, ':')) {
      endpoints->set_scheme(rmq::AddressScheme::IPv6);
    } else {
      endpoints->set_scheme(rmq::AddressScheme::DOMAIN_NAME);
    }
  }
}

void ClientImpl::fetchRouteFor(const std::string& topic, const std::function<void(const TopicRouteDataPtr&)>& cb) {
  std::string name_server = name_server_resolver_->current();
  if (name_server.empty()) {
    SPDLOG_WARN("No name server available");
    return;
  }

  auto callback = [this, topic, name_server, cb](bool ok, const TopicRouteDataPtr& route) {
    if (!ok || !route) {
      SPDLOG_WARN("Failed to resolve route for topic={} from {}", topic, name_server);
      std::string name_server_changed = name_server_resolver_->next();
      if (!name_server_changed.empty()) {
        SPDLOG_INFO("Change current name server from {} to {}", name_server, name_server_changed);
      }
      cb(nullptr);
      return;
    }

    SPDLOG_DEBUG("Apply callback of fetchRouteFor({}) since a valid route is fetched", topic);
    cb(route);
  };

  QueryRouteRequest request;
  request.mutable_topic()->set_resource_namespace(resource_namespace_);
  request.mutable_topic()->set_name(topic);
  auto endpoints = request.mutable_endpoints();
  setAccessPoint(endpoints);
  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(this, metadata);
  client_manager_->resolveRoute(name_server, metadata, request, absl::ToChronoMilliseconds(io_timeout_), callback);
}

void ClientImpl::updateRouteInfo() {
  if (State::STARTED != state_.load(std::memory_order_relaxed) &&
      State::STARTING != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state={}.", state_.load(std::memory_order_relaxed));
    return;
  }

  std::vector<std::string> topics;
  {
    absl::MutexLock lock(&topic_route_table_mtx_);
    for (const auto& entry : topic_route_table_) {
      topics.push_back(entry.first);
    }
  }

  if (!topics.empty()) {
    for (const auto& topic : topics) {
      fetchRouteFor(topic, std::bind(&ClientImpl::updateRouteCache, this, topic, std::placeholders::_1));
    }
  }

#ifdef ENABLE_TRACING
  updateTraceProvider();
#endif

  SPDLOG_DEBUG("Topic route info updated");
}

void ClientImpl::heartbeat() {
  absl::flat_hash_set<std::string> hosts;
  endpointsInUse(hosts);
  if (hosts.empty()) {
    SPDLOG_WARN("No hosts to send heartbeat to at present");
    return;
  }

  HeartbeatRequest request;
  prepareHeartbeatData(request);

  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(this, metadata);

  for (const auto& target : hosts) {
    auto callback = [target](bool ok, const HeartbeatResponse& response) {
      if (!ok) {
        SPDLOG_WARN("Failed to send heartbeat request to {}", target);
        return;
      }
      SPDLOG_DEBUG("Heartbeat to {} OK", target);
    };
    client_manager_->heartbeat(target, metadata, request, absl::ToChronoMilliseconds(io_timeout_), callback);
  }
}

void ClientImpl::onTopicRouteReady(const std::string& topic, const TopicRouteDataPtr& route) {
  updateRouteCache(topic, route);

  // Take all pending callbacks
  std::vector<std::function<void(const TopicRouteDataPtr&)>> pending_requests;
  {
    absl::MutexLock lk(&inflight_route_requests_mtx_);
    assert(inflight_route_requests_.contains(topic));
    auto& inflight_requests = inflight_route_requests_.at(topic);
    pending_requests.insert(pending_requests.end(), inflight_requests.begin(), inflight_requests.end());
    inflight_route_requests_.erase(topic);
  }

  SPDLOG_DEBUG("Apply cached callbacks with acquired route data for topic={}", topic);
  for (const auto& cb : pending_requests) {
    cb(route);
  }
}

void ClientImpl::updateRouteCache(const std::string& topic, const TopicRouteDataPtr& route) {
  if (!route || route->partitions().empty()) {
    SPDLOG_WARN("Yuck! route for {} is invalid", topic);
    return;
  }

  {
    absl::MutexLock lk(&topic_route_table_mtx_);
    if (!topic_route_table_.contains(topic)) {
      topic_route_table_.insert({topic, route});
      SPDLOG_INFO("TopicRouteData for topic={} has changed. NONE --> {}", topic, route->debugString());
    } else {
      TopicRouteDataPtr cached = topic_route_table_.at(topic);
      if (*cached != *route) {
        topic_route_table_.insert_or_assign(topic, route);
        std::string previous = cached->debugString();
        SPDLOG_INFO("TopicRouteData for topic={} has changed. {} --> {}", topic, cached->debugString(),
                    route->debugString());
      }
    }
  }
}

void ClientImpl::multiplexing(const std::string& target, const MultiplexingRequest& request) {
  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(this, metadata);
  client_manager_->multiplexingCall(target, metadata, request, absl::ToChronoMilliseconds(long_polling_timeout_),
                                    std::bind(&ClientImpl::onMultiplexingResponse, this, std::placeholders::_1));
}

void ClientImpl::onMultiplexingResponse(const InvocationContext<MultiplexingResponse>* ctx) {
  if (!ctx->status.ok()) {
    std::string remote_address = ctx->remote_address;
    auto multiplexingLater = [this, remote_address]() {
      MultiplexingRequest request;
      fillGenericPollingRequest(request);
      multiplexing(remote_address, request);
    };
    static std::string task_name = "Initiate multiplex request later";
    client_manager_->getScheduler().schedule(multiplexingLater, task_name, std::chrono::seconds(3),
                                             std::chrono::seconds(0));
    return;
  }

  switch (ctx->response.type_case()) {
  case MultiplexingResponse::TypeCase::kPrintThreadStackRequest: {
    MultiplexingRequest request;
    request.mutable_print_thread_stack_response()->mutable_common()->mutable_status()->set_code(
        google::rpc::Code::UNIMPLEMENTED);
    request.mutable_print_thread_stack_response()->set_stack_trace(
        "Print thread stack trace is not supported by C++ SDK");
    absl::flat_hash_map<std::string, std::string> metadata;
    Signature::sign(this, metadata);
    client_manager_->multiplexingCall(ctx->remote_address, metadata, request, absl::ToChronoMilliseconds(io_timeout_),
                                      std::bind(&ClientImpl::onMultiplexingResponse, this, std::placeholders::_1));
    break;
  }

  case MultiplexingResponse::TypeCase::kVerifyMessageConsumptionRequest: {
    auto data = ctx->response.verify_message_consumption_request().message();
    MQMessageExt message;
    MultiplexingRequest request;
    if (!client_manager_->wrapMessage(data, message)) {
      SPDLOG_WARN("Message to verify consumption is corrupted");
      request.mutable_verify_message_consumption_response()->mutable_common()->mutable_status()->set_code(
          google::rpc::Code::INVALID_ARGUMENT);
      request.mutable_verify_message_consumption_response()->mutable_common()->mutable_status()->set_message(
          "Message to verify is corrupted");
      multiplexing(ctx->remote_address, request);
      return;
    }
    std::string&& result = verifyMessageConsumption(message);
    request.mutable_verify_message_consumption_response()->mutable_common()->mutable_status()->set_code(
        google::rpc::Code::OK);
    request.mutable_verify_message_consumption_response()->mutable_common()->mutable_status()->set_message(result);
    multiplexing(ctx->remote_address, request);
    break;
  }

  case MultiplexingResponse::TypeCase::kResolveOrphanedTransactionRequest: {
    auto orphan = ctx->response.resolve_orphaned_transaction_request().orphaned_transactional_message();
    MQMessageExt message;
    if (client_manager_->wrapMessage(orphan, message)) {
      MessageAccessor::setTargetEndpoint(message, ctx->remote_address);
      const std::string& transaction_id = ctx->response.resolve_orphaned_transaction_request().transaction_id();
      resolveOrphanedTransactionalMessage(transaction_id, message);
    } else {
      SPDLOG_WARN("Failed to resolve orphaned transactional message, potentially caused by message-body checksum "
                  "verification failure.");
    }
    MultiplexingRequest request;
    fillGenericPollingRequest(request);
    multiplexing(ctx->remote_address, request);
    break;
  }

  case MultiplexingResponse::TypeCase::kPollingResponse: {
    MultiplexingRequest request;
    fillGenericPollingRequest(request);
    multiplexing(ctx->remote_address, request);
    break;
  }

  default: {
    SPDLOG_WARN("Unsupported multiplex type");
    MultiplexingRequest request;
    fillGenericPollingRequest(request);
    multiplexing(ctx->remote_address, request);
    break;
  }
  }
}

void ClientImpl::healthCheck() {
  std::vector<std::string> endpoints;
  {
    absl::MutexLock lk(&isolated_endpoints_mtx_);
    for (const auto& item : isolated_endpoints_) {
      endpoints.push_back(item);
    }
  }

  std::weak_ptr<ClientImpl> base(self());
  auto callback = [base](const std::string& endpoint,
                         const InvocationContext<HealthCheckResponse>* invocation_context) {
    std::shared_ptr<ClientImpl> ptr = base.lock();
    if (ptr) {
      ptr->onHealthCheckResponse(endpoint, invocation_context);
    } else {
      SPDLOG_INFO("BaseImpl has been destructed");
    }
  };

  for (const auto& endpoint : endpoints) {
    HealthCheckRequest request;
    absl::flat_hash_map<std::string, std::string> metadata;
    Signature::sign(this, metadata);
    client_manager_->healthCheck(endpoint, metadata, request, absl::ToChronoMilliseconds(io_timeout_), callback);
  }
}

void ClientImpl::schedule(const std::string& task_name, const std::function<void()>& task,
                          std::chrono::milliseconds delay) {
  client_manager_->getScheduler().schedule(task, task_name, delay, std::chrono::milliseconds(0));
}

void ClientImpl::onHealthCheckResponse(const std::string& endpoint, const InvocationContext<HealthCheckResponse>* ctx) {
  if (!ctx) {
    SPDLOG_WARN("ClientInstance does not have RPC client for {}. It might have been offline and thus cleaned",
                endpoint);
    {
      absl::MutexLock lk(&isolated_endpoints_mtx_);
      isolated_endpoints_.erase(endpoint);
    }
    return;
  }

  assert(endpoint == ctx->remote_address);

  if (ctx->status.ok()) {
    if (google::rpc::Code::OK == ctx->response.common().status().code()) {
      SPDLOG_INFO("Health check to server[host={}] passed. Move it back to active node pool", endpoint);
      absl::MutexLock lk(&isolated_endpoints_mtx_);
      isolated_endpoints_.erase(endpoint);
    } else {
      SPDLOG_INFO("Health check to server[host={}] failed due to application layer reason: {}",
                  ctx->response.common().DebugString());
    }
  } else {
    SPDLOG_INFO("Health check to server[host={}] failed due to transport layer reason: {}", endpoint,
                ctx->status.error_message());
  }
}

void ClientImpl::fillGenericPollingRequest(MultiplexingRequest& request) {
  auto&& resource_bundle = resourceBundle();
  auto polling_request = request.mutable_polling_request();
  polling_request->set_client_id(clientId());
  switch (resource_bundle.group_type) {
  case GroupType::PUBLISHER:
    polling_request->mutable_producer_group()->set_resource_namespace(resource_namespace_);
    polling_request->mutable_producer_group()->set_name(group_name_);
    break;

  case GroupType::SUBSCRIBER:
    polling_request->mutable_consumer_group()->set_resource_namespace(resource_namespace_);
    polling_request->mutable_consumer_group()->set_name(group_name_);
    break;
  }
  auto topics = polling_request->mutable_topics();
  for (const auto& item : resource_bundle.topics) {
    auto topic = new rmq::Resource();
    topic->set_resource_namespace(resource_namespace_);
    topic->set_name(item);
    topics->AddAllocated(topic);
  }
}

void ClientImpl::notifyClientTermination() {
  absl::flat_hash_set<std::string> endpoints;
  endpointsInUse(endpoints);

  Metadata metadata;
  Signature::sign(this, metadata);
  NotifyClientTerminationRequest request;
  request.mutable_group()->set_resource_namespace(resource_namespace_);
  request.mutable_group()->set_name(group_name_);
  request.set_client_id(clientId());

  for (const auto& endpoint : endpoints) {
    client_manager_->notifyClientTermination(endpoint, metadata, request, absl::ToChronoMilliseconds(io_timeout_));
  }
}

ROCKETMQ_NAMESPACE_END