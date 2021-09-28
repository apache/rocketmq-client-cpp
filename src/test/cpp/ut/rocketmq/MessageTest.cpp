#include "rocketmq/MQMessageExt.h"
#include <chrono>
#include <gtest/gtest.h>
#include <thread>

TEST(MessageTest, testExpire) {
  auto expire_time_point = std::chrono::steady_clock::now() + std::chrono::milliseconds(300);
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  EXPECT_TRUE(std::chrono::steady_clock::now() < expire_time_point);
}