#pragma once

#include <cstdint>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class ResponseCode : std::uint16_t
{
  Success = 0,
  InternalSystemError = 1,
  TooManyRequests = 2,
};

ROCKETMQ_NAMESPACE_END