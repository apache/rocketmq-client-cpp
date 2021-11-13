#include "rocketmq/RocketMQ.h"
#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

enum class MessageVersion : std::uint8_t
{
  Unset = 0,
  V1 = 1,
  V2 = 2,
};

MessageVersion messageVersionOf(std::int32_t magic_code);

ROCKETMQ_NAMESPACE_END