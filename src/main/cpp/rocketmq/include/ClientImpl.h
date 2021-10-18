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
#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <system_error>

#include "RpcClient.h"
#include "absl/strings/string_view.h"
#include "apache/rocketmq/v1/definition.pb.h"

#include "Client.h"
#include "ClientConfigImpl.h"
#include "ClientManager.h"
#include "ClientResourceBundle.h"
#include "InvocationContext.h"
#include "NameServerResolver.h"
#include "OtlpExporter.h"
#include "rocketmq/MQMessageExt.h"
#include "rocketmq/MessageListener.h"
#include "rocketmq/State.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientImpl : public ClientConfigImpl, virtual public Client {
public:
  explicit ClientImpl(absl::string_view group_name);

  ~ClientImpl() override = default;

  virtual void start();

  virtual void shutdown();

  void getRouteFor(const std::string& topic, const std::function<void(const std::error_code&, TopicRouteDataPtr)>& cb)
      LOCKS_EXCLUDED(inflight_route_requests_mtx_, topic_route_table_mtx_);

  /**
   * Gather collection of endpoints that are reachable from latest topic route
   * table.
   *
   * @param endpoints
   */
  void endpointsInUse(absl::flat_hash_set<std::string>& endpoints) override LOCKS_EXCLUDED(topic_route_table_mtx_);

  void heartbeat() override;

  bool active() override {
    State state = state_.load(std::memory_order_relaxed);
    return State::STARTING == state || State::STARTED == state;
  }

  void onRemoteEndpointRemoval(const std::vector<std::string>& hosts) override LOCKS_EXCLUDED(isolated_endpoints_mtx_);

  void healthCheck() override LOCKS_EXCLUDED(isolated_endpoints_mtx_);

  void schedule(const std::string& task_name, const std::function<void(void)>& task,
                std::chrono::milliseconds delay) override;

  void withNameServerResolver(std::shared_ptr<NameServerResolver> name_server_resolver) {
    name_server_resolver_ = std::move(name_server_resolver);
  }

  /**
   * Expose for test purpose only.
   */
  void state(State state) {
    state_.store(state, std::memory_order_relaxed);
  }

protected:
  ClientManagerPtr client_manager_;
  std::shared_ptr<OtlpExporter> exporter_;
  std::atomic<State> state_;

  absl::flat_hash_map<std::string, TopicRouteDataPtr> topic_route_table_ GUARDED_BY(topic_route_table_mtx_);
  absl::Mutex topic_route_table_mtx_ ACQUIRED_AFTER(inflight_route_requests_mtx_); // protects topic_route_table_

  absl::flat_hash_map<std::string, std::vector<std::function<void(const std::error_code&, const TopicRouteDataPtr&)>>>
      inflight_route_requests_ GUARDED_BY(inflight_route_requests_mtx_);
  absl::Mutex inflight_route_requests_mtx_ ACQUIRED_BEFORE(topic_route_table_mtx_); // Protects inflight_route_requests_
  static const char* UPDATE_ROUTE_TASK_NAME;
  std::uint32_t route_update_handle_{0};

  // Set Name Server Resolver
  std::shared_ptr<NameServerResolver> name_server_resolver_;

  absl::flat_hash_map<std::string, absl::Time> multiplexing_requests_;
  absl::Mutex multiplexing_requests_mtx_;

  absl::flat_hash_set<std::string> isolated_endpoints_ GUARDED_BY(isolated_endpoints_mtx_);
  absl::Mutex isolated_endpoints_mtx_;

  void updateRouteInfo() LOCKS_EXCLUDED(topic_route_table_mtx_);

  /**
   * Sub-class is supposed to inherit from std::enable_shared_from_this.
   */
  virtual std::shared_ptr<ClientImpl> self() = 0;

  virtual void prepareHeartbeatData(HeartbeatRequest& request) = 0;

  virtual void verifyMessageConsumption(std::string remote_address, std::string command_id, MQMessageExt message);

  /**
   * @brief Execute transaction-state-checker to commit or roll-back the orphan transactional message.
   *
   * It is no-op by default and Producer-subclass is supposed to override it.
   *
   * @param transaction_id
   * @param message
   */
  virtual void resolveOrphanedTransactionalMessage(const std::string& transaction_id, const MQMessageExt& message) {
  }

  /**
   * Concrete publisher/subscriber client is expected to fill other
   * type-specific resources.
   */
  virtual ClientResourceBundle resourceBundle() {
    ClientResourceBundle resource_bundle;
    resource_bundle.client_id = clientId();
    resource_bundle.resource_namespace = resource_namespace_;
    return resource_bundle;
  }

  void setAccessPoint(rmq::Endpoints* endpoints);

  void notifyClientTermination() override;

  void notifyClientTermination(const NotifyClientTerminationRequest& request);

  /**
   * @brief Return application developer provided message listener if this client is of PushConsumer type.
   *
   * By default, it returns nullptr such that error messages are generated and directed to server immediately.
   *
   * @return nullptr by default.
   */
  virtual MessageListener* messageListener() {
    return nullptr;
  }

private:
  /**
   * This is a low-level API that fetches route data from name server through
   * gRPC unary request/response. Once request/response is completed, either
   * timeout or response arrival in time, callback would get invoked.
   * @param topic
   * @param cb
   */
  void fetchRouteFor(const std::string& topic,
                     const std::function<void(const std::error_code&, const TopicRouteDataPtr&)>& cb);

  /**
   * Callback to execute once route data is fetched from name server.
   * @param topic
   * @param route
   */
  void onTopicRouteReady(const std::string& topic, const std::error_code& ec, const TopicRouteDataPtr& route)
      LOCKS_EXCLUDED(inflight_route_requests_mtx_);

  /**
   * Update local cache for the topic. Note, route differences are logged in
   * INFO level since route bears fundamental importance.
   *
   * @param topic
   * @param route
   */
  void updateRouteCache(const std::string& topic, const std::error_code& ec, const TopicRouteDataPtr& route)
      LOCKS_EXCLUDED(topic_route_table_mtx_);

  void pollCommand(const std::string& target);

  void onPollCommandResponse(const InvocationContext<PollCommandResponse>* ctx);

  void onHealthCheckResponse(const std::error_code& endpoint, const InvocationContext<HealthCheckResponse>* ctx)
      LOCKS_EXCLUDED(isolated_endpoints_mtx_);

  void doVerify(std::string target, std::string command_id, MQMessageExt message);
};

ROCKETMQ_NAMESPACE_END