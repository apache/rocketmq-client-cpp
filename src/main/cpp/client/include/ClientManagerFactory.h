#pragma once

#include <string>
#include <thread>

#include "ClientConfig.h"
#include "ClientManager.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "rocketmq/AdminServer.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientManagerFactory {
public:
  static ClientManagerFactory& getInstance();

  ClientManagerPtr getClientManager(const ClientConfig& client_config) LOCKS_EXCLUDED(client_manager_table_mtx_);

  // For test purpose only
  void addClientInstance(const std::string& arn, const ClientManagerPtr& client_manager)
      LOCKS_EXCLUDED(client_manager_table_mtx_);

private:
  ClientManagerFactory();

  virtual ~ClientManagerFactory();

  /**
   * Client Id --> Client Instance
   */
  absl::flat_hash_map<std::string, std::weak_ptr<ClientManager>>
      client_manager_table_ GUARDED_BY(client_manager_table_mtx_);
  absl::Mutex client_manager_table_mtx_; // protects client_manager_table_

  rocketmq::admin::AdminServer& admin_server_;
};

ROCKETMQ_NAMESPACE_END