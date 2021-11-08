#include "rocketmq/RocketMQ.h"

#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

enum class SessionState : uint8_t
{
  Created = 0,
  Connecting = 1,
  Connected = 2,
  Closing = 3,
  Closed = 4,
};

ROCKETMQ_NAMESPACE_END