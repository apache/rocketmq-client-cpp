#include "CountdownLatch.h"
#include "rocketmq/RocketMQ.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
#include <thread>

ROCKETMQ_NAMESPACE_BEGIN

TEST(CountdownLatchTest, testCountdown) {
  CountdownLatch countdown_latch(0);

  countdown_latch.increaseCount();

  auto start = std::chrono::system_clock::now();

  std::thread t([&] {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    spdlog::info("Counting down now...");
    countdown_latch.countdown();
  });

  spdlog::info("Start to await");
  countdown_latch.await();
  spdlog::info("Await over");
  auto duration = std::chrono::system_clock::now() - start;
  EXPECT_TRUE(std::chrono::duration_cast<std::chrono::seconds>(duration).count() >= 3);

  if (t.joinable()) {
    t.join();
  }
}

ROCKETMQ_NAMESPACE_END