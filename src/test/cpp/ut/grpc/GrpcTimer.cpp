
#include "absl/time/time.h"
#include "grpc/grpc.h"
#include "rocketmq/RocketMQ.h"
#include "src/core/lib/iomgr/closure.h"
#include "src/core/lib/iomgr/timer.h"
#include "gtest/gtest.h"
#include <atomic>
#include <functional>
#include <iostream>
#include <sys/stat.h>
#include <thread>

ROCKETMQ_NAMESPACE_BEGIN

struct Timer {
  grpc_timer g_timer;
  absl::Mutex mtx;
  absl::CondVar cv;
  bool invoked{false};
};

struct PeriodicTimer {
  grpc_timer g_timer;
  absl::Duration initial_delay{absl::Seconds(3)};
  absl::Duration interval{absl::Seconds(1)};
  int times{10};
};

class GrpcTimerTest : public testing::Test {
public:
  void SetUp() override { grpc_init(); }
  void TearDown() override { grpc_shutdown(); }

  static void schedule(void* arg, grpc_error_handle error) {
    auto timer = static_cast<PeriodicTimer*>(arg);
    if (error == GRPC_ERROR_CANCELLED) {
      std::cout << absl::FormatTime(absl::Now()) << ": job cancelled" << std::endl;
      delete timer;
      return;
    }

    std::cout << "Thread-" << std::this_thread::get_id() << ": Periodic Callback" << std::endl;
    grpc_core::ExecCtx exec_ctx;
    std::cout << absl::FormatTime(absl::Now()) << ": Timer->times = " << timer->times << std::endl;
    if (!--timer->times) {
      delete timer;
      std::cout << "Delete periodic timer" << std::endl;
      return;
    }

    grpc_timer_init(&timer->g_timer, grpc_core::ExecCtx::Get()->Now() + 1000,
                    GRPC_CLOSURE_CREATE(schedule, timer, grpc_schedule_on_exec_ctx));
  }
};

TEST_F(GrpcTimerTest, testSetUp) {}

TEST_F(GrpcTimerTest, testSingleShot) {
  grpc_core::ExecCtx exec_ctx;
  Timer timer;

  auto callback = [](void* arg, grpc_error_handle error) {
    std::cout << "Callback" << std::endl;
    auto t = static_cast<Timer*>(arg);
    absl::MutexLock lk(&t->mtx);
    t->invoked = true;
    t->cv.SignalAll();
  };

  grpc_timer_init(&timer.g_timer, grpc_core::ExecCtx::Get()->Now() + 500,
                  GRPC_CLOSURE_CREATE(callback, &timer, grpc_schedule_on_exec_ctx));

  std::cout << "Waiting callback invocation" << std::endl;

  while (!timer.invoked) {
    absl::MutexLock lk(&timer.mtx);
    timer.cv.Wait(&timer.mtx);
  }
  ASSERT_TRUE(timer.invoked);
}

TEST_F(GrpcTimerTest, testMultiShot) {
  grpc_core::ExecCtx exec_ctx;
  auto timer = new PeriodicTimer();
  std::cout << absl::FormatTime(absl::Now()) << ": Arm the timer" << std::endl;
  std::cout << "Now = " << grpc_core::ExecCtx::Get()->Now() << std::endl;
  grpc_timer_init(&timer->g_timer, grpc_core::ExecCtx::Get()->Now() + 3000,
                  GRPC_CLOSURE_CREATE(schedule, timer, grpc_schedule_on_exec_ctx));

  absl::SleepFor(absl::Seconds(5));
  grpc_timer_cancel(&timer->g_timer);
}

ROCKETMQ_NAMESPACE_END