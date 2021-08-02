#pragma once

#include "RocketMQ.h"
#include <functional>

ROCKETMQ_NAMESPACE_BEGIN

using Executor = std::function<void(const std::function<void()>&)>;

ROCKETMQ_NAMESPACE_END