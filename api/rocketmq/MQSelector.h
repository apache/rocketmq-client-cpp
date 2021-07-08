#pragma once

#include <vector>
#include "MQMessageQueue.h"
#include "MQMessage.h"

ROCKETMQ_NAMESPACE_BEGIN

class MessageQueueSelector {
public:
    virtual ~MessageQueueSelector() {}
    virtual MQMessageQueue select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg, void* arg) = 0;
};

ROCKETMQ_NAMESPACE_END