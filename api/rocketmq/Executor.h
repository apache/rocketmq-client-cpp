#pragma once

#include <functional>

#include "RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

using Executor = std::function<void(const std::function<void()>&)>;

ROCKETMQ_NAMESPACE_END