#pragma once

#include <cstdint>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class RemotingCommandType : std::uint8_t
{
  REQUEST = 0,
  RESPONSE = 1,
};

ROCKETMQ_NAMESPACE_END