#include "rocketmq/MQMessage.h"
#include "gtest/gtest.h"
#include <cstring>

ROCKETMQ_NAMESPACE_BEGIN

class MQMessageTest : public testing::Test {
public:
  void SetUp() override {
    message.setTopic(topic_);
    message.setKey(key_);
    message.setBody(body_data_, strlen(body_data_));
    message.setDelayTimeLevel(delay_level_);
  }

  void TearDown() override {}

protected:
  std::string topic_{"Test"};
  std::string key_{"key0"};
  const char* body_data_{"body content"};
  int delay_level_{1};
  MQMessage message;
};

TEST_F(MQMessageTest, testAssignment) {
  MQMessage msg;
  msg = message;
  EXPECT_EQ(msg.getTopic(), topic_);
  EXPECT_EQ(msg.getDelayTimeLevel(), delay_level_);
  EXPECT_EQ(*msg.getKeys().begin(), key_);
  EXPECT_EQ(msg.getBody(), body_data_);
}

TEST_F(MQMessageTest, testProperty) {
  std::string key{"k"};
  std::string value{"value"};
  message.setProperty(key, value);
  auto prop_value = message.getProperty(key);
  EXPECT_EQ(value, prop_value);
}

ROCKETMQ_NAMESPACE_END