#include "ClientImpl.h"
#include "ClientManagerFactory.h"
#include "GHttpClient.h"
#include "InvocationContext.h"
#include "LoggerImpl.h"
#include "MessageAccessor.h"
#include "Signature.h"
#include "absl/strings/str_join.h"
#include "apache/rocketmq/v1/definition.pb.h"
#include "rocketmq/MQMessageExt.h"
#include <chrono>
#include <memory>
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

ClientImpl::ClientImpl(std::string group_name) : ClientConfigImpl(std::move(group_name)), state_(State::CREATED) {}

void ClientImpl::start() {
  State expected = CREATED;
  if (!state_.compare_exchange_strong(expected, State::STARTING)) {
    SPDLOG_WARN("Unexpected state: {}", state_.load(std::memory_order_relaxed));
    return;
  }

  client_manager_ = ClientManagerFactory::getInstance().getClientManager(*this);
  client_manager_->start();
  bool update_name_server_list = false;
  {
    absl::MutexLock lock(&name_server_list_mtx_);
    if (name_server_list_.empty()) {
      update_name_server_list = true;
    }
  }

  std::weak_ptr<ClientImpl> ptr(self());

  if (update_name_server_list) {
    // Acquire name server list immediately
    renewNameServerList();

    // Schedule to renew name server list periodically
    SPDLOG_INFO("Name server list was empty. Schedule a task to fetch and renew periodically");
    auto name_server_update_functor = [ptr]() {
      std::shared_ptr<ClientImpl> base = ptr.lock();
      if (base) {
        base->renewNameServerList();
      }
    };
    name_server_update_handle_ =
        client_manager_->getScheduler().schedule(name_server_update_functor, UPDATE_NAME_SERVER_LIST_TASK_NAME,
                                                  std::chrono::milliseconds(0), std::chrono::seconds(30));
  }

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
  if (name_server_update_handle_) {
    client_manager_->getScheduler().cancel(name_server_update_handle_);
  }

  if (route_update_handle_) {
    client_manager_->getScheduler().cancel(route_update_handle_);
  }

  client_manager_.reset();
}

const char* ClientImpl::UPDATE_ROUTE_TASK_NAME = "route_updater";
const char* ClientImpl::UPDATE_NAME_SERVER_LIST_TASK_NAME = "name_server_list_updater";

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

void ClientImpl::debugNameServerChanges(const std::vector<std::string>& list) {
  std::string previous;
  bool changed = false;
  {
    absl::MutexLock lock(&name_server_list_mtx_);
    if (name_server_list_ != list) {
      changed = true;
      if (name_server_list_.empty()) {
        previous.append("[]");
      } else {
        previous = absl::StrJoin(name_server_list_.begin(), name_server_list_.end(), ";");
      }
    }
  }
  std::string current = absl::StrJoin(list.begin(), list.end(), ";");
  if (changed) {
    SPDLOG_INFO("Name server list changed. {} --> {}", previous, current);
  } else {
    SPDLOG_DEBUG("Name server list remains the same: {}", current);
  }
}

void ClientImpl::renewNameServerList() {
  if (State::STARTED != state_.load(std::memory_order_relaxed) &&
      State::STARTING != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state: {}", state_.load(std::memory_order_relaxed));
    return;
  }

  std::vector<std::string> list;
  SPDLOG_DEBUG("Begin to renew name server list");
  auto callback = [this](int code, const std::vector<std::string>& name_server_list) {
    if (GHttpClient::STATUS_OK != code) {
      SPDLOG_WARN("Failed to fetch name server list");
      return;
    }

    if (name_server_list.empty()) {
      SPDLOG_WARN("Yuck, got an empty name server list");
      return;
    }

    debugNameServerChanges(name_server_list);
    {
      absl::MutexLock lock(&name_server_list_mtx_);
      if (name_server_list_ != name_server_list) {
        name_server_list_.clear();
        name_server_list_.insert(name_server_list_.begin(), name_server_list.begin(), name_server_list.end());
      }
    }
  };
  client_manager_->topAddressing().fetchNameServerAddresses(callback);
}

bool ClientImpl::selectNameServer(std::string& selected, bool change) {
  static uint32_t index = 0;
  if (change) {
    index++;
  }
  {
    absl::MutexLock lock(&name_server_list_mtx_);
    if (name_server_list_.empty()) {
      return false;
    }
    uint32_t idx = index % name_server_list_.size();
    selected = name_server_list_[idx];
  }
  return true;
}

void ClientImpl::fetchRouteFor(const std::string& topic, const std::function<void(const TopicRouteDataPtr&)>& cb) {
  std::string name_server;
  if (!selectNameServer(name_server)) {
    SPDLOG_WARN("No name server available");
    return;
  }

  auto callback = [this, topic, name_server, cb](bool ok, const TopicRouteDataPtr& route) {
    if (!ok || !route) {
      SPDLOG_WARN("Failed to resolve route for topic={} from {}", topic, name_server);
      std::string name_server_changed;
      if (selectNameServer(name_server_changed, true)) {
        SPDLOG_INFO("Change current name server from {} to {}", name_server, name_server_changed);
      }
      cb(nullptr);
      return;
    }

    SPDLOG_DEBUG("Apply callback of fetchRouteFor({}) since a valid route is fetched", topic);
    cb(route);
  };

  QueryRouteRequest request;
  request.mutable_topic()->set_arn(arn_);
  request.mutable_topic()->set_name(topic);

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
  if (request.heartbeats().empty()) {
    SPDLOG_INFO("No heartbeat entries to send. Skip.");
    return;
  }

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
  auto protocol_bundle = request.mutable_polling_request()->mutable_client_resource_bundle();
  protocol_bundle->set_client_id(resource_bundle.client_id);
  for (auto& item : resource_bundle.topics) {
    auto topic = new rmq::Resource;
    topic->set_arn(resource_bundle.arn);
    topic->set_name(item);
    protocol_bundle->mutable_topics()->AddAllocated(topic);
  }

  switch (resource_bundle.group_type) {
  case GroupType::PUBLISHER:
    protocol_bundle->mutable_producer_group()->set_arn(resource_bundle.arn);
    protocol_bundle->mutable_producer_group()->set_name(resource_bundle.group);
    break;
  case GroupType::SUBSCRIBER:
    protocol_bundle->mutable_consumer_group()->set_arn(resource_bundle.arn);
    protocol_bundle->mutable_consumer_group()->set_name(resource_bundle.group);
    break;
  }
}

ROCKETMQ_NAMESPACE_END