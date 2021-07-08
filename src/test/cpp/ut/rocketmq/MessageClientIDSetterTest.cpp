#include "MessageClientIDSetter.h"
#include "rocketmq/RocketMQ.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
#include <set>

ROCKETMQ_NAMESPACE_BEGIN

TEST(MessageClientIDSetterTest, testMessageClientIDSetter) {
  std::set<std::string> msg_id_set;
  int64_t total = 500000;
  int64_t count = 0;

  while (total--) {
    std::string msgId = MessageClientIDSetter::createUniqId();
    if (0 == total % 10000) {
      SPDLOG_INFO("MsgId: {}", msgId);
    }
    msg_id_set.insert(msgId);
    ++count;
  }
  EXPECT_EQ(count, msg_id_set.size());
}

ROCKETMQ_NAMESPACE_END