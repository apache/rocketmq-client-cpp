/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

#include "RpcClient.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "apache/rocketmq/v1/definition.pb.h"
#include "google/rpc/code.pb.h"

#include "ClientImpl.h"
#include "ClientManagerFactory.h"
#include "HttpClientImpl.h"
#include "InvocationContext.h"
#include "LoggerImpl.h"
#include "MessageAccessor.h"
#include "Signature.h"
#include "rocketmq/MQMessageExt.h"
#include "rocketmq/MessageListener.h"

ROCKETMQ_NAMESPACE_BEGIN

ClientImpl::ClientImpl(absl::string_view group_name) : ClientConfigImpl(group_name), state_(State::CREATED) {
}

void ClientImpl::start() {
  State expected = CREATED;
  if (!state_.compare_exchange_strong(expected, State::STARTING)) {
    SPDLOG_ERROR("Attempt to start ClientImpl failed. Expecting: {} Actual: {}", State::CREATED,
                 state_.load(std::memory_order_relaxed));
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

  route_update_handle_ = client_manager_->getScheduler()->schedule(route_update_functor, UPDATE_ROUTE_TASK_NAME,
                                                                   std::chrono::seconds(10), std::chrono::seconds(30));
}

void ClientImpl::shutdown() {
  State expected = State::STOPPING;
  if (state_.compare_exchange_strong(expected, State::STOPPED)) {
    name_server_resolver_->shutdown();
    if (route_update_handle_) {
      client_manager_->getScheduler()->cancel(route_update_handle_);
    }
    client_manager_.reset();
  } else {
    SPDLOG_ERROR("Try to shutdown ClientImpl, but its state is not as expected. Expecting: {}, Actual: {}",
                 State::STOPPING, state_.load(std::memory_order_relaxed));
  }
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

void ClientImpl::getRouteFor(const std::string& topic,
                             const std::function<void(const std::error_code&, TopicRouteDataPtr)>& cb) {
  TopicRouteDataPtr route = nullptr;
  {
    absl::MutexLock lock(&topic_route_table_mtx_);
    if (topic_route_table_.contains(topic)) {
      route = topic_route_table_.at(topic);
    }
  }

  if (route) {
    std::error_code ec;
    cb(ec, route);
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
        std::vector<std::function<void(const std::error_code&, const TopicRouteDataPtr&)>> inflight{cb};
        inflight_route_requests_.insert({topic, inflight});
        SPDLOG_INFO("Create inflight route query cache for topic={}", topic);
      }
    }
  }

  if (!query_backend && route) {
    std::error_code ec;
    cb(ec, route);
  } else {
    fetchRouteFor(topic,
                  std::bind(&ClientImpl::onTopicRouteReady, this, topic, std::placeholders::_1, std::placeholders::_2));
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

void ClientImpl::fetchRouteFor(const std::string& topic,
                               const std::function<void(const std::error_code&, const TopicRouteDataPtr&)>& cb) {
  std::string name_server = name_server_resolver_->current();
  if (name_server.empty()) {
    SPDLOG_WARN("No name server available");
    return;
  }

  auto callback = [this, topic, name_server, cb](const std::error_code& ec, const TopicRouteDataPtr& route) {
    if (ec) {
      SPDLOG_WARN("Failed to resolve route for topic={} from {}", topic, name_server);
      std::string name_server_changed = name_server_resolver_->next();
      if (!name_server_changed.empty()) {
        SPDLOG_INFO("Change current name server from {} to {}", name_server, name_server_changed);
      }
      cb(ec, nullptr);
      return;
    }

    SPDLOG_DEBUG("Apply callback of fetchRouteFor({}) since a valid route is fetched", topic);
    cb(ec, route);
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
      fetchRouteFor(
          topic, std::bind(&ClientImpl::updateRouteCache, this, topic, std::placeholders::_1, std::placeholders::_2));
    }
  }
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
    auto callback = [target](const std::error_code& ec, const HeartbeatResponse& response) {
      if (ec) {
        SPDLOG_WARN("Failed to heartbeat against {}. Cause: {}", target, ec.message());
        return;
      }
      SPDLOG_DEBUG("Heartbeat to {} OK", target);
    };
    client_manager_->heartbeat(target, metadata, request, absl::ToChronoMilliseconds(io_timeout_), callback);
  }
}

void ClientImpl::onTopicRouteReady(const std::string& topic, const std::error_code& ec,
                                   const TopicRouteDataPtr& route) {
  if (route) {
    SPDLOG_DEBUG("Received route data for topic={}", topic);
  }

  updateRouteCache(topic, ec, route);

  // Take all pending callbacks
  std::vector<std::function<void(const std::error_code&, const TopicRouteDataPtr&)>> pending_requests;
  {
    absl::MutexLock lk(&inflight_route_requests_mtx_);
    assert(inflight_route_requests_.contains(topic));
    auto& inflight_requests = inflight_route_requests_.at(topic);
    pending_requests.insert(pending_requests.end(), inflight_requests.begin(), inflight_requests.end());
    inflight_route_requests_.erase(topic);
  }

  SPDLOG_DEBUG("Apply cached callbacks with acquired route data for topic={}", topic);
  for (const auto& cb : pending_requests) {
    cb(ec, route);
  }
}

void ClientImpl::updateTraceHosts() {
  absl::flat_hash_set<std::string> hosts;
  absl::MutexLock lk(&topic_route_table_mtx_);
  for (const auto& item : topic_route_table_) {
    for (const auto& partition : item.second->partitions()) {
      if (Permission::NONE == partition.permission()) {
        continue;
      }
      if (MixAll::MASTER_BROKER_ID != partition.broker().id()) {
        continue;
      }
      std::string endpoint = partition.asMessageQueue().serviceAddress();
      if (!hosts.contains(endpoint)) {
        hosts.emplace(std::move(endpoint));
      }
    }
  }
  std::vector<std::string> host_list(hosts.begin(), hosts.end());
  SPDLOG_DEBUG("Trace candidate hosts size={}", host_list.size());
  exporter_->updateHosts(host_list);
}

void ClientImpl::updateRouteCache(const std::string& topic, const std::error_code& ec, const TopicRouteDataPtr& route) {
  if (ec || !route || route->partitions().empty()) {
    SPDLOG_WARN("Yuck! route for {} is invalid. Cause: {}", topic, ec.message());
    return;
  }

  absl::flat_hash_set<std::string> new_hosts;
  {
    absl::MutexLock lk(&topic_route_table_mtx_);
    absl::flat_hash_set<std::string> existed_hosts;
    for (const auto& item : topic_route_table_) {
      for (const auto& partition : item.second->partitions()) {
        std::string endpoint = partition.asMessageQueue().serviceAddress();
        if (!existed_hosts.contains(endpoint)) {
          existed_hosts.emplace(std::move(endpoint));
        }
      }
    }
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
    absl::flat_hash_set<std::string> hosts;
    for (const auto& item : topic_route_table_) {
      for (const auto& partition : item.second->partitions()) {
        std::string endpoint = partition.asMessageQueue().serviceAddress();
        if (!hosts.contains(endpoint)) {
          hosts.emplace(std::move(endpoint));
        }
      }
    }
    std::set_difference(hosts.begin(), hosts.end(), existed_hosts.begin(), existed_hosts.end(),
                        std::inserter(new_hosts, new_hosts.begin()));
  }
  updateTraceHosts();
  for (const auto& endpoints : new_hosts) {
    pollCommand(endpoints);
  }
}

void ClientImpl::pollCommand(const std::string& target) {
  SPDLOG_INFO("Start to poll command to remote, target={}", target);
  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(this, metadata);

  PollCommandRequest request;
  auto&& resource_bundle = resourceBundle();
  request.set_client_id(resource_bundle.client_id);
  switch (resource_bundle.group_type) {
    case GroupType::PUBLISHER:
      request.mutable_producer_group()->set_resource_namespace(resource_namespace_);
      request.mutable_producer_group()->set_name(group_name_);
      break;

    case GroupType::SUBSCRIBER:
      request.mutable_consumer_group()->set_resource_namespace(resource_namespace_);
      request.mutable_consumer_group()->set_name(group_name_);
      break;
  }
  auto topics = request.mutable_topics();
  for (const auto& item : resource_bundle.topics) {
    auto topic = new rmq::Resource();
    topic->set_resource_namespace(resource_namespace_);
    topic->set_name(item);
    topics->AddAllocated(topic);
  }

  client_manager_->pollCommand(target, metadata, request, absl::ToChronoMilliseconds(long_polling_timeout_),
                               std::bind(&ClientImpl::onPollCommandResponse, this, std::placeholders::_1));
}

void ClientImpl::verifyMessageConsumption(std::string remote_address, std::string command_id, MQMessageExt message) {
  SPDLOG_INFO("Received message to verify consumption, messageId={}", message.getMsgId());
  MessageListener* listener = messageListener();

  Metadata metadata;
  Signature::sign(this, metadata);
  ReportMessageConsumptionResultRequest request;
  request.set_command_id(command_id);

  if (!listener) {
    request.mutable_status()->set_code(google::rpc::Code::FAILED_PRECONDITION);
    request.mutable_status()->set_message("Target is not a push consumer client");
    client_manager_->reportMessageConsumptionResult(remote_address, metadata, request,
                                                    absl::ToChronoMilliseconds(io_timeout_));
    return;
  }

  if (MessageListenerType::FIFO == listener->listenerType()) {
    request.mutable_status()->set_code(google::rpc::Code::FAILED_PRECONDITION);
    request.mutable_status()->set_message("FIFO message does NOT support verification of message consumption");
    client_manager_->reportMessageConsumptionResult(remote_address, metadata, request,
                                                    absl::ToChronoMilliseconds(io_timeout_));
    return;
  }

  // Execute the actual verification task in dedicated thread-pool.
  client_manager_->submit(std::bind(&ClientImpl::doVerify, this, remote_address, command_id, message));
}

void ClientImpl::onPollCommandResponse(const InvocationContext<PollCommandResponse>* ctx) {
  std::string address = ctx->remote_address;
  absl::flat_hash_set<std::string> hosts;
  endpointsInUse(hosts);
  if (!hosts.contains(address)) {
    SPDLOG_INFO("Endpoint={} is now absent from route table. Break poll-command-cycle.", address);
    return;
  }
  if (!ctx->status.ok()) {
    static std::string task_name = "Poll-Command-Later";
    client_manager_->getScheduler()->schedule(std::bind(&ClientImpl::pollCommand, this, ctx->remote_address), task_name,
                                              std::chrono::seconds(3), std::chrono::seconds(0));
    return;
  }

  switch (ctx->response.type_case()) {
    case PollCommandResponse::TypeCase::kPrintThreadStackTraceCommand: {
      absl::flat_hash_map<std::string, std::string> metadata;
      Signature::sign(this, metadata);
      ReportThreadStackTraceRequest request;
      auto command_id = ctx->response.print_thread_stack_trace_command().command_id();
      request.set_command_id(command_id);
      request.set_thread_stack_trace("--RocketMQ-Client-CPP does NOT support thread stack trace report--");
      client_manager_->reportThreadStackTrace(ctx->remote_address, metadata, request,
                                              absl::ToChronoMilliseconds(io_timeout_));
      break;
    }

    case PollCommandResponse::TypeCase::kVerifyMessageConsumptionCommand: {
      auto command_id = ctx->response.verify_message_consumption_command().command_id();
      auto data = ctx->response.verify_message_consumption_command().message();
      MQMessageExt message;
      ReportMessageConsumptionResultRequest request;
      request.set_command_id(command_id);
      Metadata metadata;
      Signature::sign(this, metadata);
      if (!client_manager_->wrapMessage(data, message)) {
        SPDLOG_WARN("Message to verify consumption is corrupted");
        request.mutable_status()->set_code(google::rpc::Code::INVALID_ARGUMENT);
        request.mutable_status()->set_message("Data corrupted");
        client_manager_->reportMessageConsumptionResult(ctx->remote_address, metadata, request,
                                                        absl::ToChronoMilliseconds(io_timeout_));
      }
      verifyMessageConsumption(std::move(ctx->remote_address), std::move(command_id), std::move(message));
      break;
    }

    case PollCommandResponse::TypeCase::kRecoverOrphanedTransactionCommand: {
      auto orphan = ctx->response.recover_orphaned_transaction_command().orphaned_transactional_message();
      MQMessageExt message;
      if (client_manager_->wrapMessage(orphan, message)) {
        MessageAccessor::setTargetEndpoint(message, ctx->remote_address);
        const std::string& transaction_id = ctx->response.recover_orphaned_transaction_command().transaction_id();
        // Dispatch task to thread-pool.
        client_manager_->submit(
            std::bind(&ClientImpl::resolveOrphanedTransactionalMessage, this, transaction_id, message));
      } else {
        SPDLOG_WARN("Failed to resolve orphaned transactional message, potentially caused by message-body checksum "
                    "verification failure.");
      }
      break;
    }

    case PollCommandResponse::TypeCase::kNoopCommand: {
      SPDLOG_DEBUG("A long-polling-command period completed.");
      break;
    }

    default: {
      SPDLOG_WARN("Unsupported multiplex type");
      break;
    }
  }

  // Initiate next round of long-polling-command immediately.
  pollCommand(ctx->remote_address);
}

void ClientImpl::onRemoteEndpointRemoval(const std::vector<std::string>& hosts) {
  absl::MutexLock lk(&isolated_endpoints_mtx_);
  for (auto it = isolated_endpoints_.begin(); it != isolated_endpoints_.end();) {
    if (hosts.end() != std::find_if(hosts.begin(), hosts.end(), [&](const std::string& item) { return *it == item; })) {
      SPDLOG_INFO("Drop isolated-endoint[{}] as it has been removed from route table", *it);
      isolated_endpoints_.erase(it++);
    } else {
      it++;
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
  auto callback = [base](const std::error_code& ec, const InvocationContext<HealthCheckResponse>* invocation_context) {
    std::shared_ptr<ClientImpl> ptr = base.lock();
    if (!ptr) {
      SPDLOG_INFO("BaseImpl has been destructed");
      return;
    }

    ptr->onHealthCheckResponse(ec, invocation_context);
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
  client_manager_->getScheduler()->schedule(task, task_name, delay, std::chrono::milliseconds(0));
}

void ClientImpl::onHealthCheckResponse(const std::error_code& ec, const InvocationContext<HealthCheckResponse>* ctx) {
  if (ec) {
    SPDLOG_WARN("Health check to server[host={}] failed. Cause: {}", ec.message());
    return;
  }

  SPDLOG_INFO("Health check to server[host={}] passed. Remove it from isolated endpoint pool", ctx->remote_address);
  {
    absl::MutexLock lk(&isolated_endpoints_mtx_);
    isolated_endpoints_.erase(ctx->remote_address);
  }
}

void ClientImpl::notifyClientTermination() {
  SPDLOG_WARN("Should NOT reach here. Subclass should have overridden this function.");
  std::abort();
}

void ClientImpl::notifyClientTermination(const NotifyClientTerminationRequest& request) {
  absl::flat_hash_set<std::string> endpoints;
  endpointsInUse(endpoints);

  Metadata metadata;
  Signature::sign(this, metadata);

  for (const auto& endpoint : endpoints) {
    client_manager_->notifyClientTermination(endpoint, metadata, request, absl::ToChronoMilliseconds(io_timeout_));
  }
}

void ClientImpl::doVerify(std::string target, std::string command_id, MQMessageExt message) {
  ReportMessageConsumptionResultRequest request;
  request.set_command_id(command_id);
  StandardMessageListener* callback = reinterpret_cast<StandardMessageListener*>(messageListener());
  try {
    std::vector<MQMessageExt> batch = {message};
    auto result = callback->consumeMessage(batch);
    switch (result) {
      case ConsumeMessageResult::SUCCESS: {
        SPDLOG_DEBUG("Verify message[MsgId={}] OK", message.getMsgId());
        request.mutable_status()->set_message("Consume Success");
        break;
      }
      case ConsumeMessageResult::FAILURE: {
        SPDLOG_WARN("Message Listener failed to consume message[MsgId={}] when verifying", message.getMsgId());
        request.mutable_status()->set_code(google::rpc::Code::INTERNAL);
        request.mutable_status()->set_message("Consume Failed");
        break;
      }
    }
  } catch (...) {
    SPDLOG_WARN("Exception raised when invoking message listener provided by application developer. MsgId of message "
                "to verify: {}",
                message.getMsgId());
    request.mutable_status()->set_code(google::rpc::Code::INTERNAL);
    request.mutable_status()->set_message(
        "Unexpected exception raised while invoking message listener provided by application developer");
  }

  Metadata metadata;
  Signature::sign(this, metadata);
  client_manager_->reportMessageConsumptionResult(target, metadata, request, absl::ToChronoMilliseconds(io_timeout_));
}

ROCKETMQ_NAMESPACE_END