#pragma once

#include "ConsumerMock.h"
#include "PushConsumer.h"
#include <system_error>

ROCKETMQ_NAMESPACE_BEGIN

class PushConsumerMock : virtual public PushConsumer, virtual public ConsumerMock {
public:
  MOCK_METHOD(void, iterateProcessQueue, (const std::function<void(ProcessQueueSharedPtr)>&), (override));

  MOCK_METHOD(MessageModel, messageModel, (), (const override));

  MOCK_METHOD(void, ack, (const MQMessageExt&, const std::function<void(const std::error_code&)>&), (override));

  MOCK_METHOD(void, forwardToDeadLetterQueue, (const MQMessageExt&, const std::function<void(bool)>&), (override));

  MOCK_METHOD(const Executor&, customExecutor, (), (const override));

  MOCK_METHOD(uint32_t, consumeBatchSize, (), (const override));

  MOCK_METHOD(int32_t, maxDeliveryAttempts, (), (const override));

  MOCK_METHOD(void, updateOffset, (const MQMessageQueue&, int64_t), (override));

  MOCK_METHOD(void, nack, (const MQMessageExt&, const std::function<void(const std::error_code&)>&), (override));

  MOCK_METHOD(std::shared_ptr<ConsumeMessageService>, getConsumeMessageService, (), (override));

  MOCK_METHOD(bool, receiveMessage, (const MQMessageQueue&, const FilterExpression&), (override));

  MOCK_METHOD(MessageListener*, messageListener, (), (override));
};

ROCKETMQ_NAMESPACE_END