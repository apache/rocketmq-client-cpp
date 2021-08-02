#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "ConsumerMock.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(ConsumerMockTest, testMock) {
  testing::NiceMock<ConsumerMock> mock;
}

ROCKETMQ_NAMESPACE_END