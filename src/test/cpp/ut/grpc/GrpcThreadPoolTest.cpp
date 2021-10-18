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