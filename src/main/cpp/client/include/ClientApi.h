#pragma once

#include <string>
#include <vector>

#include "RpcClient.h"
#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"

namespace rocketmq {
class ClientApi {
public:
  ClientApi() = default;

  void update_name_server_list(const std::vector<std::string>& list) LOCKS_EXCLUDED(name_server_list_mtx_);

private:
  std::vector<std::string> name_server_list_ GUARDED_BY(name_server_list_mtx_);
  absl::Mutex name_server_list_mtx_; // Protects name_server_list_
};
} // namespace rocketmq