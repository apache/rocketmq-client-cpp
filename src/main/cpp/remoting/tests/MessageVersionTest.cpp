#include <cstdint>

#include "gtest/gtest.h"

#include "MessageVersion.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(MessageVersionTest, testVersion) {
  std::int32_t code1 = -626843481;
  std::int32_t code2 = -626843477;

  MessageVersion v1 = messageVersionOf(code1);
  MessageVersion v2 = messageVersionOf(code2);
  ASSERT_EQ(v1, MessageVersion::V1);
  ASSERT_EQ(v2, MessageVersion::V2);
}

ROCKETMQ_NAMESPACE_END