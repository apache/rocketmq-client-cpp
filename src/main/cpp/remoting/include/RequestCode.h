#pragma once

#include <cstdint>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class RequestCode : std::int32_t
{
  SendMessage = 10,
  PullMessage = 11,
  QueryRoute = 105,  
};

ROCKETMQ_NAMESPACE_END