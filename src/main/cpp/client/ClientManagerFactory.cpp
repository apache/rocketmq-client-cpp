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
    auto search = client_manager_table_.find(client_config.resourceNamespace());
    if (search != client_manager_table_.end()) {
      ClientManagerPtr client_manager = search->second.lock();
      if (client_manager) {
        SPDLOG_DEBUG("Re-use existing client_manager[resource_namespace={}]", client_config.resourceNamespace());
        return client_manager;
      } else {
        client_manager_table_.erase(client_config.resourceNamespace());
      }
    }
    ClientManagerPtr client_manager = std::make_shared<ClientManagerImpl>(client_config.resourceNamespace());
    std::weak_ptr<ClientManager> client_instance_weak_ptr(client_manager);
    client_manager_table_.insert_or_assign(client_config.resourceNamespace(), client_instance_weak_ptr);
    SPDLOG_INFO("Created a new client manager[resource_namespace={}]", client_config.resourceNamespace());
    return client_manager;
  }
}

void ClientManagerFactory::addClientManager(const std::string& resource_namespace,
                                            const ClientManagerPtr& client_manager) {
  absl::MutexLock lk(&client_manager_table_mtx_);
  client_manager_table_.insert_or_assign(resource_namespace, std::weak_ptr<ClientManager>(client_manager));
}

ClientManagerFactory::ClientManagerFactory() : admin_server_(admin::AdminFacade::getServer()) { admin_server_.start(); }

ClientManagerFactory::~ClientManagerFactory() { admin_server_.stop(); }

ROCKETMQ_NAMESPACE_END