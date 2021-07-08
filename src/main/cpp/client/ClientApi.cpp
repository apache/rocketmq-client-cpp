#include "ClientApi.h"

using namespace rocketmq;

void ClientApi::update_name_server_list(const std::vector<std::string>& list) {
  if (list.empty()) {
    return;
  }

  {
    absl::MutexLock lock(&name_server_list_mtx_);
    name_server_list_.clear();
    name_server_list_.insert(name_server_list_.begin(), list.begin(), list.end());
  }
}