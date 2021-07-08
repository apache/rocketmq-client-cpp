#include "BaseImpl.h"
#include "ClientManager.h"
#include "Signature.h"
#include "absl/strings/str_join.h"

ROCKETMQ_NAMESPACE_BEGIN

BaseImpl::BaseImpl(std::string group_name)
    : ClientConfig(std::move(group_name)), state_(State::CREATED), top_addressing_(absl::make_unique<TopAddressing>()) {
  topic_route_info_updater_ = std::bind(&BaseImpl::updateRouteInfo, this);
  topic_route_info_updater_function_ = std::make_shared<Functional>(&topic_route_info_updater_);

  name_server_list_updater_ = std::bind(&BaseImpl::renewNameServerList, this);
  name_server_list_updater_function_ = std::make_shared<Functional>(&name_server_list_updater_);
}

void BaseImpl::start() {
  State expected = CREATED;
  if (!state_.compare_exchange_strong(expected, State::STARTING)) {
    SPDLOG_WARN("Unexpected state: {}", state_.load(std::memory_order_relaxed));
    return;
  }

  client_instance_ = ClientManager::getInstance().getClientInstance(*this);
  client_instance_->start();
  top_addressing_->setUnitName(unit_name_);
  bool update_name_server_list = false;
  {
    absl::MutexLock lock(&name_server_list_mtx_);
    if (name_server_list_.empty()) {
      update_name_server_list = true;
    }
  }

  if (update_name_server_list) {
    // Acquire name server list immediately
    renewNameServerList();

    // Schedule to renew name server list periodically
    SPDLOG_INFO("Name server list was empty. Schedule a task to fetch and renew periodically");
    bool scheduled = client_instance_->getScheduler().schedule(name_server_list_updater_function_,
                                                               std::chrono::milliseconds(0), std::chrono::seconds(30));
    if (scheduled) {
      SPDLOG_INFO("Name server list updater scheduled");
    } else {
      SPDLOG_ERROR("Failed to schedule name server list updater");
    }
  }

  bool scheduled = client_instance_->getScheduler().schedule(topic_route_info_updater_function_,
                                                             std::chrono::seconds(10), std::chrono::seconds(30));
  if (scheduled) {
    SPDLOG_INFO("Topic route info updater scheduled");
  } else {
    SPDLOG_ERROR("Failed to schedule topic route info updater");
  }
  state_.store(State::STARTED);
}

void BaseImpl::activeHosts(absl::flat_hash_set<std::string>& hosts) {
  absl::MutexLock lk(&topic_route_table_mtx_);
  for (const auto& item : topic_route_table_) {
    for (const auto& partition : item.second->partitions()) {
      std::string endpoint = partition.asMessageQueue().serviceAddress();
      if (!hosts.contains(endpoint)) {
        hosts.emplace(std::move(endpoint));
      }
    }
  }
}

void BaseImpl::getRouteFor(const std::string& topic, const std::function<void(TopicRouteDataPtr)>& cb) {
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
    fetchRouteFor(topic, std::bind(&BaseImpl::onTopicRouteReady, this, topic, std::placeholders::_1));
  }
}

void BaseImpl::debugNameServerChanges(const std::vector<std::string>& list) {
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

void BaseImpl::renewNameServerList() {
  if (State::STARTED != state_.load(std::memory_order_relaxed) &&
      State::STARTING != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected client instance state: {}", state_.load(std::memory_order_relaxed));
    return;
  }

  std::vector<std::string> list;
  SPDLOG_DEBUG("Begin to renew name server list");
  if (!top_addressing_->fetchNameServerAddresses(list)) {
    SPDLOG_WARN("Failed to fetch name server list");
  } else {
    if (list.empty()) {
      SPDLOG_WARN("Yuck, got an empty name server list");
    } else {
      debugNameServerChanges(list);
      {
        absl::MutexLock lock(&name_server_list_mtx_);
        if (name_server_list_ != list) {
          name_server_list_.clear();
          name_server_list_.insert(name_server_list_.begin(), list.begin(), list.end());
        }
        {
          absl::MutexLock promise_lock(&name_server_promise_list_mtx_);
          for (auto& promise : name_server_promise_list_) {
            promise.set_value(name_server_list_);
          }
          name_server_promise_list_.clear();
        }
      }
    }
  }
}

bool BaseImpl::selectNameServer(std::string& selected, bool change) {
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

void BaseImpl::fetchRouteFor(const std::string& topic, const std::function<void(const TopicRouteDataPtr&)>& cb) {
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
  client_instance_->resolveRoute(name_server, metadata, request, absl::ToChronoMilliseconds(io_timeout_), callback);
}

void BaseImpl::updateRouteInfo() {
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
      fetchRouteFor(topic, std::bind(&BaseImpl::updateRouteCache, this, topic, std::placeholders::_1));
    }
  }

#ifdef ENABLE_TRACING
  updateTraceProvider();
#endif

  SPDLOG_DEBUG("Topic route info updated");
}

void BaseImpl::heartbeat() {
  absl::flat_hash_set<std::string> hosts;
  activeHosts(hosts);
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
    client_instance_->heartbeat(target, metadata, request, absl::ToChronoMilliseconds(io_timeout_), callback);
  }
}

void BaseImpl::onTopicRouteReady(const std::string& topic, const TopicRouteDataPtr& route) {
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

void BaseImpl::updateRouteCache(const std::string& topic, const TopicRouteDataPtr& route) {
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

ROCKETMQ_NAMESPACE_END