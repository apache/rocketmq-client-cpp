#pragma once

#include "rocketmq/MessageListener.h"
#include "gmock/gmock.h"

ROCKETMQ_NAMESPACE_BEGIN

class StandardMessageListenerMock : public StandardMessageListener {
public:
  MOCK_METHOD(ConsumeMessageResult, consumeMessage, (const std::vector<MQMessageExt>&), (override));
};

class FifoMessageListenerMock : public FifoMessageListener {
public:
  MOCK_METHOD(ConsumeMessageResult, consumeMessage, (const MQMessageExt&), (override));
};

ROCKETMQ_NAMESPACE_END