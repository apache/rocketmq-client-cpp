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