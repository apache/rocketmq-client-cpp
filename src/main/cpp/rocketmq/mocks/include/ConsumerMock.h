#pragma once

#include "ClientMock.h"
#include "Consumer.h"

ROCKETMQ_NAMESPACE_BEGIN

class ConsumerMock : virtual public Consumer, virtual public ClientMock {
public:
  MOCK_METHOD((absl::flat_hash_map<std::string, FilterExpression>), getTopicFilterExpressionTable, (), (const override));

  MOCK_METHOD(uint32_t, maxCachedMessageQuantity, (), (const override));

  MOCK_METHOD(uint64_t, maxCachedMessageMemory, (), (const override));

  MOCK_METHOD(int32_t, receiveBatchSize, (), (const override));
};

ROCKETMQ_NAMESPACE_END