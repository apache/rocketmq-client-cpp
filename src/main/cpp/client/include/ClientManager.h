#pragma once

#include <string>
#include <thread>

#include "ClientConfig.h"
#include "ClientInstance.h"
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "rocketmq/AdminServer.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientManager {
public:
  static ClientManager& getInstance();

  ClientInstancePtr getClientInstance(const ClientConfig& client_config) LOCKS_EXCLUDED(client_instance_table_mtx_);

  // For test purpose only
  void addClientInstance(const std::string& arn, const ClientInstancePtr& client_instance)
      LOCKS_EXCLUDED(client_instance_table_mtx_);

private:
  ClientManager();

  virtual ~ClientManager();

  /**
   * Client Id --> Client Instance
   */
  absl::flat_hash_map<std::string, std::weak_ptr<ClientInstance>>
      client_instance_table_ GUARDED_BY(client_instance_table_mtx_);
  absl::Mutex client_instance_table_mtx_; // protects client_instance_table_

  rocketmq::admin::AdminServer& admin_server_;
};

ROCKETMQ_NAMESPACE_END