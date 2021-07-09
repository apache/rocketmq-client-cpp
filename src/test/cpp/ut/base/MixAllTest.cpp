#include "MixAll.h"
#include "rocketmq/MQMessage.h"
#include "gtest/gtest.h"

using namespace rocketmq;

class MixAllTest : public testing::Test {};

TEST_F(MixAllTest, testValidate_empty_topic) {
  MQMessage message;
  ASSERT_FALSE(MixAll::validate(message));
}

TEST_F(MixAllTest, testValidate_normal_topic) {
  MQMessage message("T_abc-123", "sample_body");
  ASSERT_TRUE(MixAll::validate(message));
}

TEST_F(MixAllTest, testValidate_topic_too_long) {

  std::string topic("T");
  for (int i = 0;; ++i) {
    topic.append(std::to_string(i));
    if (topic.length() > 64) {
      break;
    }
  }
  MQMessage message(topic, "sample_body");
  ASSERT_FALSE(MixAll::validate(message));
}

TEST_F(MixAllTest, testValidate_body_too_large) {
  std::string topic("TestTopic");
  std::string body;

  body.reserve(MixAll::MAX_MESSAGE_BODY_SIZE + 1);
  for (uint32_t i = 0; i <= MixAll::MAX_MESSAGE_BODY_SIZE; ++i) {
    body.append("a");
  }
  ASSERT_FALSE(MixAll::validate(MQMessage(topic, body)));
}

TEST_F(MixAllTest, testRandom) {
  uint32_t left = 1;
  uint32_t right = 100;
  uint32_t random_number = MixAll::random(left, right);
  EXPECT_TRUE(random_number >= left && random_number <= right);
}

TEST_F(MixAllTest, testHex) {
  const char* data = "abc";
  std::string hex = MixAll::hex(data, strlen(data));
  std::vector<uint8_t> bin;
  EXPECT_TRUE(MixAll::hexToBinary(hex, bin));
  EXPECT_EQ(hex, MixAll::hex(bin.data(), bin.size()));
}