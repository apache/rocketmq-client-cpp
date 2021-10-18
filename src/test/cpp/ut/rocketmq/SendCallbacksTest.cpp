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
#include "SendCallbacks.h"
#include "gtest/gtest.h"
#include <thread>

ROCKETMQ_NAMESPACE_BEGIN

class SendCallbacksTest : public testing::Test {};

TEST_F(SendCallbacksTest, testAwait) {
  std::string msg_id("Msg-0");
  AwaitSendCallback callback;
  std::thread waker([&]() {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    SendResult send_result;
    send_result.setMsgId(msg_id);
    callback.onSuccess(send_result);
  });

  callback.await();
  EXPECT_TRUE(callback);
  EXPECT_EQ(callback.sendResult().getMsgId(), msg_id);
  if (waker.joinable()) {
    waker.join();
  }
}

ROCKETMQ_NAMESPACE_END