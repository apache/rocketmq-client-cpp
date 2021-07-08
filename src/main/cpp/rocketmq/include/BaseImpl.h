#include "ClientInstance.h"
#include "rocketmq/State.h"

ROCKETMQ_NAMESPACE_BEGIN

class BaseImpl : public ClientConfig, public ClientCallback {
public:
  explicit BaseImpl(std::string group_name);

  ~BaseImpl() override = default;

  virtual void start();

  void getRouteFor(const std::string& topic, const std::function<void(TopicRouteDataPtr)>& cb)
      LOCKS_EXCLUDED(inflight_route_requests_mtx_, topic_route_table_mtx_);

  void activeHosts(absl::flat_hash_set<std::string>& hosts) override LOCKS_EXCLUDED(topic_route_table_mtx_);

  void setNameServerList(std::vector<std::string> name_server_list) {
    absl::MutexLock lk(&name_server_list_mtx_);
    name_server_list_ = std::move(name_server_list);
  }

  virtual void prepareHeartbeatData(HeartbeatRequest& request) = 0;

  void heartbeat() override;

protected:
  ClientInstancePtr client_instance_;
  std::atomic<State> state_;

  absl::flat_hash_map<std::string, TopicRouteDataPtr> topic_route_table_ GUARDED_BY(topic_route_table_mtx_);
  absl::Mutex topic_route_table_mtx_ ACQUIRED_AFTER(inflight_route_requests_mtx_); // protects topic_route_table_

  absl::flat_hash_map<std::string, std::vector<std::function<void(const TopicRouteDataPtr&)>>>
      inflight_route_requests_ GUARDED_BY(inflight_route_requests_mtx_);
  absl::Mutex inflight_route_requests_mtx_ ACQUIRED_BEFORE(topic_route_table_mtx_); // Protects inflight_route_requests_

  std::function<void(void)> topic_route_info_updater_;
  FunctionalSharePtr topic_route_info_updater_function_;

  // Name server list management
  std::vector<std::string> name_server_list_ GUARDED_BY(name_server_list_mtx_);
  absl::Mutex name_server_list_mtx_; // protects name_server_list_

  std::function<void(void)> name_server_list_updater_;
  FunctionalSharePtr name_server_list_updater_function_;

  std::string unit_name_;
  TopAddressingPtr top_addressing_;

  std::vector<std::promise<std::vector<std::string>>> name_server_promise_list_;
  absl::Mutex name_server_promise_list_mtx_ ACQUIRED_AFTER(name_server_list_mtx_);

  void debugNameServerChanges(const std::vector<std::string>& list) LOCKS_EXCLUDED(name_server_list_mtx_);

  void renewNameServerList() LOCKS_EXCLUDED(name_server_list_mtx_);

  bool selectNameServer(std::string& selected, bool change = false) LOCKS_EXCLUDED(name_server_list_mtx_);

  void updateRouteInfo() LOCKS_EXCLUDED(topic_route_table_mtx_);

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
};

ROCKETMQ_NAMESPACE_END