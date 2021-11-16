#pragma once

#include "rocketmq/RocketMQ.h"
#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

enum class ConsumeFromWhere : std::uint8_t
{
  ConsumeFromLastOffset = 0,
  ConsumeFromFirstOffset = 1,
  ConsumeFromTimestamp = 2,
};

ROCKETMQ_NAMESPACE_END