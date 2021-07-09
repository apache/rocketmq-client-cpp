#include <chrono>
#include <cstdint>
#include <functional>
#include <iostream>
#include <thread>

#include "Scheduler.h"
#include "grpc/grpc.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class SchedulerTest : public testing::Test {
public:
  void SetUp() override {
    grpc_init();
    callback_fire_count = 0;

    callback = [this]() {
      absl::MutexLock lock(&mtx);
      cv.Signal();
      callback_fire_count++;
    };
  }

  void TearDown() override { grpc_shutdown(); }

protected:
  Scheduler scheduler;
  absl::Mutex mtx;
  absl::CondVar cv;
  int callback_fire_count{0};

  std::function<void(void)> callback;
};

TEST_F(SchedulerTest, testSingleShot) {
  scheduler.schedule(callback, "single-shot", std::chrono::milliseconds(10), std::chrono::milliseconds(0));

  // Wait till callback is executed.
  {
    absl::MutexLock lock(&mtx);
    EXPECT_TRUE(!cv.WaitWithTimeout(&mtx, absl::Seconds(3)));
  }
}

TEST_F(SchedulerTest, testCancel) {
  std::uintptr_t task_id =
      scheduler.schedule(callback, "test-cancel", std::chrono::seconds(1), std::chrono::seconds(1));
  scheduler.cancel(task_id);
  std::this_thread::sleep_for(std::chrono::seconds(2));
  ASSERT_EQ(0, callback_fire_count);
}

TEST_F(SchedulerTest, testPeriodicShot) {
  std::uintptr_t task_id =
      scheduler.schedule(callback, "periodic-task", std::chrono::milliseconds(10), std::chrono::seconds(1));
  // Wait till callback is executed.
  std::this_thread::sleep_for(std::chrono::seconds(5));
  ASSERT_TRUE(callback_fire_count >= 4);
  scheduler.cancel(task_id);
}

ROCKETMQ_NAMESPACE_END