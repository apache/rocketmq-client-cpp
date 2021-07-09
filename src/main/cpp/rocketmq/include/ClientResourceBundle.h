#include "rocketmq/RocketMQ.h"

#include <string>
#include <vector>

ROCKETMQ_NAMESPACE_BEGIN

enum class GroupType : int8_t {
  PUBLISHER  = 0,
  SUBSCRIBER = 1,
};

struct ClientResourceBundle {
  std::string client_id;
  std::vector<std::string> topics;
  std::string arn;
  std::string group;
  GroupType group_type{GroupType::PUBLISHER};
};

ROCKETMQ_NAMESPACE_END