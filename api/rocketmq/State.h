#pragma once

#include <cstdint>

#include "RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum State : uint8_t { CREATED = 0, STARTING = 1, STARTED = 2, STOPPING = 3, STOPPED = 4 };

ROCKETMQ_NAMESPACE_END