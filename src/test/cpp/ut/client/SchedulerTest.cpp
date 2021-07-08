#include <chrono>
#include <iostream>
#include <thread>

#include "Functional.h"
#include "Scheduler.h"
#include "gtest/gtest.h"

#include "absl/synchronization/mutex.h"

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class SchedulerTest : public testing::Test {
public:
  void SetUp() override {
    spdlog::set_level(spdlog::level::debug);
    std::cout << "SetUp invoked" << std::endl;
  }

  void TearDown() override { std::cout << "TearDown invoked" << std::endl; }

protected:
  Scheduler scheduler;
};

TEST_F(SchedulerTest, testStartShutdown) {
  scheduler.start();
  scheduler.shutdown();
}

TEST_F(SchedulerTest, testStartReentrancy) {
  scheduler.start();
  scheduler.start();
  scheduler.shutdown();
}

TEST_F(SchedulerTest, testShutdownReentrancy) {
  scheduler.start();
  scheduler.shutdown();
  scheduler.shutdown();
}

TEST_F(SchedulerTest, testShutdownWithoutStart) { scheduler.shutdown(); }

absl::Mutex mtx;
absl::CondVar cv;
int callback_fire_count = 0;

std::function<void(void)> callback = []() {
  {
    absl::MutexLock lock(&mtx);
    cv.Signal();
    callback_fire_count++;
  }
  std::cout << "It works from callback" << std::endl;
};

TEST_F(SchedulerTest, testScheduler) {
  scheduler.start();
  FunctionalSharePtr functor = std::make_shared<Functional>(&callback);

  bool scheduled = scheduler.schedule(functor, std::chrono::milliseconds(10), std::chrono::milliseconds(0));

  EXPECT_TRUE(scheduled);

  // Wait till callback is executed.
  {
    absl::MutexLock lock(&mtx);
    EXPECT_TRUE(!cv.WaitWithTimeout(&mtx, absl::Seconds(3)));
  }

  scheduler.shutdown();
}

TEST_F(SchedulerTest, testCancel) {
  scheduler.start();
  FunctionalSharePtr functor = std::make_shared<Functional>(&callback);
  bool scheduled = scheduler.schedule(functor, std::chrono::milliseconds(10), std::chrono::seconds(1));
  EXPECT_TRUE(scheduled);
  std::this_thread::sleep_for(std::chrono::seconds(3));
  bool cancelled = scheduler.cancel(&callback);
  EXPECT_TRUE(cancelled);

  {
    absl::MutexLock lock(&mtx);
    callback_fire_count = 0;
  }

  std::this_thread::sleep_for(std::chrono::seconds(3));
  ASSERT_EQ(0, callback_fire_count);

  scheduler.shutdown();
}

TEST_F(SchedulerTest, testSchedulerManyTimes) {
  scheduler.start();
  FunctionalSharePtr functor = std::make_shared<Functional>(&callback);
  bool scheduled = scheduler.schedule(functor, std::chrono::milliseconds(0), std::chrono::seconds(1));
  EXPECT_TRUE(scheduled);

  // Wait till callback is executed.
  std::this_thread::sleep_for(std::chrono::seconds(5));

  scheduler.shutdown();
}

ROCKETMQ_NAMESPACE_END