/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "RateLimiter.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"
#include <iostream>
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN
class RateLimiterTest : public ::testing::Test {
public:
  void TearDown() override {
    observer.stop();
  }

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