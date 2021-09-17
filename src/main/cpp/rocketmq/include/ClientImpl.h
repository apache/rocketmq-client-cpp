#pragma once

#include <chrono>
#include <cstdint>

#include "apache/rocketmq/v1/definition.pb.h"

#include "Client.h"
#include "ClientConfigImpl.h"
#include "ClientManager.h"
#include "ClientResourceBundle.h"
#include "InvocationContext.h"
#include "NameServerResolver.h"
#include "OtlpExporter.h"
#include "rocketmq/MQMessageExt.h"
#include "rocketmq/State.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientImpl : virtual public Client, virtual public ClientConfigImpl {
public:
  explicit ClientImpl(std::string group_name);

  ~ClientImpl() override = default;

  virtual void start();

  virtual void shutdown();

  void getRouteFor(const std::string& topic, const std::function<void(TopicRouteDataPtr)>& cb)
      LOCKS_EXCLUDED(inflight_route_requests_mtx_, topic_route_table_mtx_);

  /**
   * Gather collection of endpoints that are reachable from latest topic route table.
   *
   * @param endpoints
   */
  void endpointsInUse(absl::flat_hash_set<std::string>& endpoints) override LOCKS_EXCLUDED(topic_route_table_mtx_);

  void heartbeat() override;

  bool active() override {
    State state = state_.load(std::memory_order_relaxed);
    return State::STARTING == state || State::STARTED == state;
  }

  void healthCheck() LOCKS_EXCLUDED(isolated_endpoints_mtx_) override;

  void schedule(const std::string& task_name, const std::function<void(void)>& task,
                std::chrono::milliseconds delay) override;

  void withNameServerResolver(std::shared_ptr<NameServerResolver> name_server_resolver) {
    name_server_resolver_ = std::move(name_server_resolver);
  }

protected:
  ClientManagerPtr client_manager_;
  std::shared_ptr<OtlpExporter> exporter_;
  std::atomic<State> state_;

  absl::flat_hash_map<std::string, TopicRouteDataPtr> topic_route_table_ GUARDED_BY(topic_route_table_mtx_);
  absl::Mutex topic_route_table_mtx_ ACQUIRED_AFTER(inflight_route_requests_mtx_); // protects topic_route_table_

  absl::flat_hash_map<std::string, std::vector<std::function<void(const TopicRouteDataPtr&)>>>
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

  virtual std::string verifyMessageConsumption(const MQMessageExt& message) { return "Unsupported"; }

  virtual void resolveOrphanedTransactionalMessage(const std::string& transaction_id, const MQMessageExt& message) {}

  /**
   * Concrete publisher/subscriber client is expected to fill other type-specific resources.
   */
  virtual ClientResourceBundle resourceBundle() {
    ClientResourceBundle resource_bundle;
    resource_bundle.client_id = clientId();
    resource_bundle.resource_namespace = resource_namespace_;
    return resource_bundle;
  }

  void setAccessPoint(rmq::Endpoints* endpoints);

  virtual void notifyClientTermination();

private:
  /**
   * This is a low-level API that fetches route data from name server through gRPC unary request/response. Once
   * request/response is completed, either timeout or response arrival in time, callback would get invoked.
   * @param topic
   * @param cb
   */
  void fetchRouteFor(const std::string& topic, const std::function<void(const TopicRouteDataPtr&)>& cb);

  /**
   * Callback to execute once route data is fetched from name server.
   * @param topic
   * @param route
   */
  void onTopicRouteReady(const std::string& topic, const TopicRouteDataPtr& route)
      LOCKS_EXCLUDED(inflight_route_requests_mtx_);

  /**
   * Update local cache for the topic. Note, route differences are logged in INFO level since route bears fundamental
   * importance.
   *
   * @param topic
   * @param route
   */
  void updateRouteCache(const std::string& topic, const TopicRouteDataPtr& route)
      LOCKS_EXCLUDED(topic_route_table_mtx_);

  void multiplexing(const std::string& target, const MultiplexingRequest& request);

  void onMultiplexingResponse(const InvocationContext<MultiplexingResponse>* ctx);

  void onHealthCheckResponse(const std::string& endpoint, const InvocationContext<HealthCheckResponse>* ctx)
      LOCKS_EXCLUDED(isolated_endpoints_mtx_);

  void fillGenericPollingRequest(MultiplexingRequest& request);
};

ROCKETMQ_NAMESPACE_END