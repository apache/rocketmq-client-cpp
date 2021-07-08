#include "ProcessQueue.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class ProcessQueueTest : public testing::Test {};

TEST_F(ProcessQueueTest, testRequestId) {
  std::weak_ptr<DefaultMQPushConsumerImpl> owner;
  ProcessQueue process_queue(MQMessageQueue(), FilterExpression("*"), ConsumeMessageType::POP, 1, owner, nullptr);
  std::string&& request_id = process_queue.requestId();
  std::vector<uint8_t> bin;
  EXPECT_TRUE(MixAll::hexToBinary(request_id, bin));
  EXPECT_EQ(request_id, MixAll::hex(bin.data(), bin.size()));
}

ROCKETMQ_NAMESPACE_END