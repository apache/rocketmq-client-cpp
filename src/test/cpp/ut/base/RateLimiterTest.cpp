#include "RateLimiter.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"
#include <iostream>
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN
class RateLimiterTest : public ::testing::Test {
public:
  void TearDown() override { observer.stop(); }

protected:
  RateLimiterObserver observer;
};

TEST_F(RateLimiterTest, basicTest) {
  std::shared_ptr<RateLimiter<10>> limiter(new RateLimiter<10>(1000));
  observer.subscribe(limiter);

  std::atomic_bool stopped(false);
  std::atomic_long acquired(0);

  std::thread timer([&] {
    std::this_thread::sleep_for(std::chrono::seconds(10));
    stopped.store(true);
  });

  std::thread report([&] {
    while (!stopped) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      long qps = acquired.load();
      while (!acquired.compare_exchange_weak(qps, 0)) {
        qps = acquired.load();
      }
      std::cout << "QPS: " << qps << ", available=" << limiter->available() << std::endl;
    }
  });

  std::thread t([&] {
    while (!stopped.load()) {
      limiter->acquire();
      acquired++;
    }
  });

  timer.join();
  t.join();
  report.join();
}

ROCKETMQ_NAMESPACE_END