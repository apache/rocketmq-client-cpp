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
#include "ResponseFuture.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "InvokeCallback.h"
#include "RemotingCommand.h"
#include "UtilAll.h"
#include "protocol/RequestCode.h"

using ::testing::_;
using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::InvokeCallback;
using rocketmq::MQRequestCode;
using rocketmq::RemotingCommand;
using rocketmq::ResponseFuture;
using rocketmq::UtilAll;

class MockInvokeCallback : public InvokeCallback {
 public:
  void operationComplete(ResponseFuture* responseFuture) noexcept {}
};

TEST(ResponseFutureTest, Init) {
  ResponseFuture responseFuture(MQRequestCode::QUERY_BROKER_OFFSET, 4, 1000);
  EXPECT_EQ(responseFuture.request_code(), MQRequestCode::QUERY_BROKER_OFFSET);
  EXPECT_EQ(responseFuture.opaque(), 4);
  EXPECT_EQ(responseFuture.timeout_millis(), 1000);
  EXPECT_FALSE(responseFuture.send_request_ok());
  EXPECT_FALSE(responseFuture.hasInvokeCallback());

  // ~ResponseFuture delete callback
  ResponseFuture twoResponseFuture(MQRequestCode::QUERY_BROKER_OFFSET, 4, 1000,
                                   std::unique_ptr<InvokeCallback>(new MockInvokeCallback()));
  EXPECT_TRUE(twoResponseFuture.hasInvokeCallback());
}

TEST(ResponseFutureTest, Info) {
  ResponseFuture responseFuture(MQRequestCode::QUERY_BROKER_OFFSET, 4, 1000);

  responseFuture.set_send_request_ok(true);
  EXPECT_TRUE(responseFuture.send_request_ok());
}

TEST(ResponseFutureTest, Response) {
  ResponseFuture responseFuture(MQRequestCode::QUERY_BROKER_OFFSET, 4, 1000);
  EXPECT_FALSE(responseFuture.hasInvokeCallback());

  std::unique_ptr<RemotingCommand> responseCommand(new RemotingCommand());
  responseFuture.setResponseCommand(std::move(responseCommand));
  EXPECT_EQ(responseFuture.getResponseCommand()->code(), 0);

  ResponseFuture responseFuture2(MQRequestCode::QUERY_BROKER_OFFSET, 4, 1000);
  uint64_t millis = UtilAll::currentTimeMillis();
  auto remotingCommand = responseFuture2.waitResponse(1000);
  uint64_t useTime = UtilAll::currentTimeMillis() - millis;
  EXPECT_LT(useTime, 3000);
  EXPECT_EQ(remotingCommand, nullptr);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "ResponseFutureTest.*";
  return RUN_ALL_TESTS();
}
