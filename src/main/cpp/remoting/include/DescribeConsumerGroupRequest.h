#pragma once

#include <string>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

struct DescribeConsumerGroupRequest {

  std::string group_;
};

ROCKETMQ_NAMESPACE_END