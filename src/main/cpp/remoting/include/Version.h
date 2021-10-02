#pragma once

#include <cstdint>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class Version : std::int32_t
{
  V4_9_1 = 396,
  V4_9_2 = 398,
};

ROCKETMQ_NAMESPACE_END