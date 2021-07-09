#include "absl/memory/memory.h"
#include "absl/synchronization/mutex.h"
#include "rocketmq/RocketMQ.h"
#include "src/cpp/server/dynamic_thread_pool.h"
#include "src/cpp/server/thread_pool_interface.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

TEST(GrpcThreadPoolTest, testSetUp) {
  auto thread_pool = std::unique_ptr<grpc::ThreadPoolInterface>(grpc::CreateDefaultThreadPool());

  absl::Mutex mtx;
  absl::CondVar cv;

  bool invoked = false;

  auto callback = [&]() {
    absl::MutexLock lk(&mtx);
    invoked = true;
    cv.SignalAll();
  };

  thread_pool->Add(callback);

  if (!invoked) {
    absl::MutexLock lk(&mtx);
    cv.WaitWithTimeout(&mtx, absl::Seconds(3));
  }

  ASSERT_TRUE(invoked);
}

ROCKETMQ_NAMESPACE_END