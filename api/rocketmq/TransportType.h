#pragma once

#include <cstdint>

#include "RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class TransportType : std::uint8_t
{
  Grpc = 0,
  Remoting = 1,
};

ROCKETMQ_NAMESPACE_END