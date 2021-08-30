#include "ThreadPool.h"
#include "gtest/gtest.h"
#include "rocketmq/RocketMQ.h"
#include "absl/memory/memory.h"

ROCKETMQ_NAMESPACE_BEGIN

class ThreadPoolTest : public testing::Test {
public:
  void SetUp() override {
    pool_ = absl::make_unique<ThreadPool>(2);
  }
  
protected:
  std::unique_ptr<ThreadPool> pool_;
};

TEST_F(ThreadPoolTest, testBasics) {
  auto task = [](int cnt) {
    for (int i = 0; i < cnt; i++) {
      std::cout << "It works" << std::endl;
    }
  };
  pool_->enqueue(task, 3);
}

ROCKETMQ_NAMESPACE_END


