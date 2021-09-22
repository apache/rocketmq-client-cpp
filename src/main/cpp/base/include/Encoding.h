#pragma once

#include <cstdint>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class Encoding : int8_t {
  IDENTITY = 0,
  GZIP = 1,
  SNAPPY = 2,
};

ROCKETMQ_NAMESPACE_END