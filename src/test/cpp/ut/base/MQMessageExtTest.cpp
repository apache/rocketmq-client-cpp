#include "rocketmq/MQMessageExt.h"
#include "gtest/gtest.h"
#include <chrono>

ROCKETMQ_NAMESPACE_BEGIN

class MQMessageExtTest : public testing::Test {
public:
  void SetUp() override {}

  void TearDown() override {}

protected:
  MQMessageExt message_;
};

TEST_F(MQMessageExtTest, testGetQueueId) {
  EXPECT_EQ(message_.getQueueId(), 0);
}

TEST_F(MQMessageExtTest, testBornTimestamp) {
  auto born_timestamp = message_.bornTimestamp();
  EXPECT_TRUE(std::chrono::system_clock::now() - born_timestamp < std::chrono::seconds(1));
}

TEST_F(MQMessageExtTest, testGetDeliveryAttempt) { EXPECT_EQ(message_.getDeliveryAttempt(), 0); }

ROCKETMQ_NAMESPACE_END