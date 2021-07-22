#include "gtest/gtest.h"
#include "MessageGroupQueueSelector.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(MessageGroupQueueSelectorTest, testSelect) {
  std::string message_group("sample");
  std::size_t hash_code = std::hash<std::string>{}(message_group);
  MessageGroupQueueSelector selector(message_group);
  std::size_t len = 8;
  std::vector<MQMessageQueue> mqs;
  mqs.resize(len);
  for (std::size_t i = 0; i < len; i++) {
    MQMessageQueue queue("topic", "broker-a", static_cast<int>(i));
    mqs.emplace_back(queue);
  }

  MQMessage message;
  MQMessageQueue selected = selector.select(mqs, message, nullptr);
  EXPECT_EQ(selected, mqs[hash_code % len]);
}

ROCKETMQ_NAMESPACE_END