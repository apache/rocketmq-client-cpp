#pragma once

#include <cstdint>

#include "RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class MessageType : int8_t {
  NORMAL = 0,
  FIFO = 1,
  DELAY = 2,
  TRANSACTION = 3,
};

ROCKETMQ_NAMESPACE_END