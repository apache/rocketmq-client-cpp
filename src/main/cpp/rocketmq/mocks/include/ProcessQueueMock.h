#include "ProcessQueue.h"
#include "gmock/gmock.h"

ROCKETMQ_NAMESPACE_BEGIN

class ProcessQueueMock : public ProcessQueue {
public:
  MOCK_METHOD(bool, expired, (), (const override));

  MOCK_METHOD(void, callback, (std::shared_ptr<ReceiveMessageCallback>), (override));

  MOCK_METHOD(void, receiveMessage, (), (override));

  MOCK_METHOD(void, nextOffset, (int64_t), (override));

  MOCK_METHOD(bool, hasPendingMessages, (), (const override));

  MOCK_METHOD(std::string, topic, (), (const override));

  MOCK_METHOD(bool, take, (uint32_t, (std::vector<MQMessageExt>&)), (override));

  MOCK_METHOD(std::weak_ptr<PushConsumer>, getConsumer, (), (override));

  MOCK_METHOD(const std::string&, simpleName, (), (const override));

  MOCK_METHOD(bool, committedOffset, (int64_t&), (override));

  MOCK_METHOD(void, release, (uint64_t, int64_t), (override));

  MOCK_METHOD(ConsumeMessageType, consumeType, (), (const override));

  MOCK_METHOD(void, cacheMessages, (const std::vector<MQMessageExt>&), (override));

  MOCK_METHOD(bool, shouldThrottle, (), (const override));

  MOCK_METHOD((std::shared_ptr<ClientManager>), getClientManager, (), (override));

  MOCK_METHOD(void, syncIdleState, (), (override));

  MOCK_METHOD(const FilterExpression&, getFilterExpression, (), (const override));

  MOCK_METHOD(bool, bindFifoConsumeTask, (), (override));

  MOCK_METHOD(bool, unbindFifoConsumeTask, (), (override));

  MOCK_METHOD(MQMessageQueue, getMQMessageQueue, (), (override));
};

ROCKETMQ_NAMESPACE_END