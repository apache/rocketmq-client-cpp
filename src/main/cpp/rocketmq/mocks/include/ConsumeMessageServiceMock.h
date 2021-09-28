#pragma once

#include "ConsumeMessageService.h"

#include "gmock/gmock.h"

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class ConsumeMessageServiceMock : public ConsumeMessageService {
public:
  MOCK_METHOD(void, start, (), (override));

  MOCK_METHOD(void, shutdown, (), (override));

  MOCK_METHOD(void, submitConsumeTask, (const ProcessQueueWeakPtr&), (override));

  MOCK_METHOD(MessageListenerType, messageListenerType, (), (override));

  MOCK_METHOD(void, signalDispatcher, (), (override));

  MOCK_METHOD(void, throttle, (const std::string&, std::uint32_t), (override));
};

ROCKETMQ_NAMESPACE_END