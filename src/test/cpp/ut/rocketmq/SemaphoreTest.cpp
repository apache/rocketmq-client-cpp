#include "Semaphore.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
#include <thread>

using namespace rocketmq;

TEST(SempahoreTest, testSemaphore) {
  Semaphore limiter(0);
  std::thread t([&] {
    auto start = std::chrono::system_clock::now();
    spdlog::info("Start to acquire");
    limiter.acquire();
    spdlog::info("Acquired");
    auto duration = std::chrono::system_clock::now() - start;
    EXPECT_TRUE(std::chrono::duration_cast<std::chrono::seconds>(duration).count() >= 1);
  });

  std::this_thread::sleep_for(std::chrono::seconds(1));
  spdlog::info("Releasing token...");
  limiter.release();
  spdlog::info("Token released");

  if (t.joinable()) {
    t.join();
  }
}