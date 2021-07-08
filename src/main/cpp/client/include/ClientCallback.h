#include "rocketmq/RocketMQ.h"
#include "absl/container/flat_hash_set.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientCallback {
public:
  virtual ~ClientCallback() = default;

  virtual void activeHosts(absl::flat_hash_set<std::string>& hosts) = 0;

  virtual void heartbeat() = 0;
};

ROCKETMQ_NAMESPACE_END