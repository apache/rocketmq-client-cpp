#include "ConsumerMock.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(ConsumerMockTest, testMock) {
  testing::NiceMock<ConsumerMock> mock;
}

ROCKETMQ_NAMESPACE_END