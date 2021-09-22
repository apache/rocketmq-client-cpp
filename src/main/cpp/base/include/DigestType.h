#pragma once

#include <cstdint>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class DigestType : int8_t {
  CRC32 = 0,
  MD5 = 1,
  SHA1 = 2,
};

ROCKETMQ_NAMESPACE_END