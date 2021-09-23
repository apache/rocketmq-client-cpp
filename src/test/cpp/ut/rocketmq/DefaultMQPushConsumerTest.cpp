#include "rocketmq/DefaultMQPushConsumer.h"

#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class DefaultMQPushConsumerTest : public testing::Test {
public:
  DefaultMQPushConsumerTest() : group_name_("group-0"), consumer_(group_name_) {}

protected:
  std::string group_name_;
  DefaultMQPushConsumer consumer_;
};

TEST_F(DefaultMQPushConsumerTest, testGroupName) { EXPECT_EQ(group_name_, consumer_.groupName()); }

ROCKETMQ_NAMESPACE_END