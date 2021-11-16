#pragma once

#include "rocketmq/RocketMQ.h"
#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

enum class ConsumeType : std::uint8_t
{
  Unset = 0,
  // PULL
  ConsumeActively = 1,
  // PUSH
  ConsumePassively = 2,
  // POP
  ConsumePop = 3,
};

ROCKETMQ_NAMESPACE_END