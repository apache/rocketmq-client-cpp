#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <thread>

#include "SchedulerImpl.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class SchedulerTest : public testing::Test {
public:
  void SetUp() override {}

protected:
};

TEST_F(SchedulerTest, testSingleShot) {
  SchedulerImpl scheduler;
  scheduler.start();
  absl::Mutex mtx;
  absl::CondVar cv;
  int callback_fire_count{0};
  auto callback = [&]() {
    absl::MutexLock lock(&mtx);
    cv.Signal();
    callback_fire_count++;
  };

  scheduler.schedule(callback, "single-shot", std::chrono::milliseconds(10), std::chrono::milliseconds(0));

  // Wait till callback is executed.
  {
    absl::MutexLock lock(&mtx);
    if (!callback_fire_count) {
      cv.Wait(&mtx);
    }
  }

  scheduler.shutdown();
}

TEST_F(SchedulerTest, testCancel) {
  SchedulerImpl scheduler;
  scheduler.start();
  absl::Mutex mtx;
  absl::CondVar cv;
  int callback_fire_count{0};
  auto callback = [&]() {
    absl::MutexLock lock(&mtx);
    cv.Signal();
    callback_fire_count++;
  };

  std::uint32_t task_id = scheduler.schedule(callback, "test-cancel", std::chrono::seconds(1), std::chrono::seconds(1));
  scheduler.cancel(task_id);
  std::this_thread::sleep_for(std::chrono::seconds(2));
  ASSERT_EQ(0, callback_fire_count);
  scheduler.shutdown();
}

TEST_F(SchedulerTest, testPeriodicShot) {
  SchedulerImpl scheduler;
  scheduler.start();
  absl::Mutex mtx;
  absl::CondVar cv;
  int callback_fire_count{0};
  auto callback = [&]() {
    absl::MutexLock lock(&mtx);
    cv.Signal();
    callback_fire_count++;
  };

  std::uintptr_t task_id =
      scheduler.schedule(callback, "periodic-task", std::chrono::milliseconds(10), std::chrono::seconds(1));
  // Wait till callback is executed.
  std::this_thread::sleep_for(std::chrono::seconds(5));
  ASSERT_TRUE(callback_fire_count >= 4);
  scheduler.cancel(task_id);
  scheduler.shutdown();
}

TEST_F(SchedulerTest, testSingleShotWithZeroDelay) {
  SchedulerImpl scheduler;
  scheduler.start();
  absl::Mutex mtx;
  absl::CondVar cv;
  int callback_fire_count{0};
  auto callback = [&]() {
    absl::MutexLock lock(&mtx);
    cv.Signal();
    callback_fire_count++;
  };

  scheduler.schedule(callback, "single-shot-with-0-delay", std::chrono::milliseconds(0), std::chrono::milliseconds(0));

  // Wait till callback is executed.
  {
    absl::MutexLock lock(&mtx);
    if (!callback_fire_count) {
      cv.Wait(&mtx);
    }
  }

  scheduler.shutdown();
}

ROCKETMQ_NAMESPACE_END