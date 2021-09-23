#pragma once

#include <vector>

#include "MQMessage.h"
#include "MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

class MessageQueueSelector {
public:
  virtual ~MessageQueueSelector() = default;

  virtual MQMessageQueue select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg, void* arg) = 0;
};

ROCKETMQ_NAMESPACE_END