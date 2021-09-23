#pragma once

#include "RocketMQ.h"

#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

enum class MessageModel : int8_t {
  BROADCASTING,
  CLUSTERING,
};

ROCKETMQ_NAMESPACE_END