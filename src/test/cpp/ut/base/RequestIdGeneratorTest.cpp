#include "RequestIdGenerator.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class RequestIdGeneratorTest : public testing::Test {};

TEST_F(RequestIdGeneratorTest, testNextRequestId) {
  std::string request_id = RequestIdGenerator::nextRequestId();
  ASSERT_TRUE(!request_id.empty());
  EXPECT_EQ(1, RequestIdGenerator::sequenceOf(request_id));
  std::cout << request_id << std::endl;
}

ROCKETMQ_NAMESPACE_END