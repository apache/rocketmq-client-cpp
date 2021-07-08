#include "ClientManager.h"

ROCKETMQ_NAMESPACE_BEGIN

ClientManager& ClientManager::getInstance() {
  static ClientManager instance;
  return instance;
}

ClientInstancePtr ClientManager::getClientInstance(const ClientConfig& client_config) {
  {
    absl::MutexLock lock(&client_instance_table_mtx_);
    auto search = client_instance_table_.find(client_config.arn());
    if (search != client_instance_table_.end()) {
      ClientInstancePtr client_instance = search->second.lock();
      if (client_instance) {
        SPDLOG_DEBUG("Re-use existing client_instance[ARN={}]", client_config.arn());
        return client_instance;
      } else {
        client_instance_table_.erase(client_config.arn());
      }
    }
    ClientInstancePtr client_instance = std::make_shared<ClientInstance>(client_config.arn());
    std::weak_ptr<ClientInstance> client_instance_weak_ptr(client_instance);
    client_instance_table_.insert_or_assign(client_config.arn(), client_instance_weak_ptr);
    SPDLOG_INFO("Created new client_instance[ARN={}]", client_config.arn());
    return client_instance;
  }
}

void ClientManager::addClientInstance(const std::string& arn, const ClientInstancePtr& client_instance) {
  absl::MutexLock lk(&client_instance_table_mtx_);
  client_instance_table_.insert_or_assign(arn, std::weak_ptr<ClientInstance>(client_instance));
}

ClientManager::ClientManager() : admin_server_(admin::AdminFacade::getServer()) { admin_server_.start(); }

ClientManager::~ClientManager() { admin_server_.stop(); }

ROCKETMQ_NAMESPACE_END