#pragma once

#include <cstdint>

#include "RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum ExpressionType : int8_t { TAG = 0, SQL92 = 1 };

ROCKETMQ_NAMESPACE_END