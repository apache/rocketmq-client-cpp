#include "ClientManagerFactory.h"
#include "ClientManagerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

ClientManagerFactory& ClientManagerFactory::getInstance() {
  static ClientManagerFactory instance;
  return instance;
}

ClientManagerPtr ClientManagerFactory::getClientManager(const ClientConfig& client_config) {
  {
    absl::MutexLock lock(&client_manager_table_mtx_);
    auto search = client_manager_table_.find(client_config.arn());
    if (search != client_manager_table_.end()) {
      ClientManagerPtr client_instance = search->second.lock();
      if (client_instance) {
        SPDLOG_DEBUG("Re-use existing client_instance[ARN={}]", client_config.arn());
        return client_instance;
      } else {
        client_manager_table_.erase(client_config.arn());
      }
    }
    ClientManagerPtr client_instance = std::make_shared<ClientManagerImpl>(client_config.arn());
    std::weak_ptr<ClientManager> client_instance_weak_ptr(client_instance);
    client_manager_table_.insert_or_assign(client_config.arn(), client_instance_weak_ptr);
    SPDLOG_INFO("Created new client_instance[ARN={}]", client_config.arn());
    return client_instance;
  }
}

void ClientManagerFactory::addClientInstance(const std::string& arn, const ClientManagerPtr& client_manager) {
  absl::MutexLock lk(&client_manager_table_mtx_);
  client_manager_table_.insert_or_assign(arn, std::weak_ptr<ClientManager>(client_manager));
}

ClientManagerFactory::ClientManagerFactory() : admin_server_(admin::AdminFacade::getServer()) { admin_server_.start(); }

ClientManagerFactory::~ClientManagerFactory() { admin_server_.stop(); }

ROCKETMQ_NAMESPACE_END