#pragma once

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class CommandCustomHeader {
public:
  virtual ~CommandCustomHeader() = default;
};

ROCKETMQ_NAMESPACE_END