#pragma once

#include "rocketmq/RocketMQ.h"

#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

enum class ConsumeMessageType : int8_t {
  ACTIVE = 0,
  PASSIVE = 1,
};

ROCKETMQ_NAMESPACE_END