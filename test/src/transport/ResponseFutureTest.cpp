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
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "AsyncCallback.h"
#include "AsyncCallbackWrap.h"
#include "MQClientAPIImpl.h"
#include "MQMessage.h"
#include "RemotingCommand.h"
#include "ResponseFuture.h"
#include "TcpRemotingClient.h"
#include "UtilAll.h"

using ::testing::_;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::AsyncCallback;
using rocketmq::asyncCallBackStatus;
using rocketmq::asyncCallBackType;
using rocketmq::AsyncCallbackWrap;
using rocketmq::MQClientAPIImpl;
using rocketmq::MQMessage;
using rocketmq::RemotingCommand;
using rocketmq::ResponseFuture;
using rocketmq::SendCallbackWrap;
using rocketmq::TcpRemotingClient;
using rocketmq::UtilAll;

class MockAsyncCallbackWrap : public SendCallbackWrap {
 public:
  MockAsyncCallbackWrap(AsyncCallback* pAsyncCallback, MQClientAPIImpl* pclientAPI)
      : SendCallbackWrap("", MQMessage(), pAsyncCallback, pclientAPI) {}

  MOCK_METHOD2(operationComplete, void(ResponseFuture*, bool));
  MOCK_METHOD0(onException, void());
  asyncCallBackType getCallbackType() { return asyncCallBackType::sendCallbackWrap; }
};

TEST(responseFuture, init) {
  ResponseFuture responseFuture(13, 4, NULL, 1000);
  EXPECT_EQ(responseFuture.getRequestCode(), 13);
  EXPECT_EQ(responseFuture.getOpaque(), 4);

  EXPECT_EQ(responseFuture.getRequestCommand().getCode(), 0);
  EXPECT_FALSE(responseFuture.isSendRequestOK());
  EXPECT_EQ(responseFuture.getMaxRetrySendTimes(), 1);
  EXPECT_EQ(responseFuture.getRetrySendTimes(), 1);
  EXPECT_EQ(responseFuture.getBrokerAddr(), "");

  EXPECT_FALSE(responseFuture.getASyncFlag());
  EXPECT_TRUE(responseFuture.getAsyncResponseFlag());
  EXPECT_FALSE(responseFuture.getSyncResponseFlag());
  EXPECT_TRUE(responseFuture.getAsyncCallbackWrap() == nullptr);

  // ~ResponseFuture  delete pcall
  SendCallbackWrap* pcall = new SendCallbackWrap("", MQMessage(), nullptr, nullptr);
  ResponseFuture twoResponseFuture(13, 4, nullptr, 1000, true, pcall);
  EXPECT_TRUE(twoResponseFuture.getASyncFlag());
  EXPECT_FALSE(twoResponseFuture.getAsyncResponseFlag());
  EXPECT_TRUE(twoResponseFuture.getSyncResponseFlag());
  EXPECT_FALSE(twoResponseFuture.getAsyncCallbackWrap() == nullptr);
}

TEST(responseFuture, info) {
  ResponseFuture responseFuture(13, 4, NULL, 1000);

  responseFuture.setAsyncResponseFlag();
  EXPECT_TRUE(responseFuture.getAsyncResponseFlag());

  responseFuture.setBrokerAddr("127.0.0.1:9876");
  EXPECT_EQ(responseFuture.getBrokerAddr(), "127.0.0.1:9876");

  responseFuture.setMaxRetrySendTimes(3000);
  EXPECT_EQ(responseFuture.getMaxRetrySendTimes(), 3000);

  responseFuture.setRetrySendTimes(3000);
  EXPECT_EQ(responseFuture.getRetrySendTimes(), 3000);

  responseFuture.setSendRequestOK(true);
  EXPECT_TRUE(responseFuture.isSendRequestOK());
}

TEST(responseFuture, response) {
  // m_bAsync = false  m_syncResponse
  ResponseFuture responseFuture(13, 4, NULL, 1000);

  EXPECT_FALSE(responseFuture.getASyncFlag());
  EXPECT_FALSE(responseFuture.getSyncResponseFlag());
  EXPECT_TRUE(responseFuture.getAsyncResponseFlag());

  RemotingCommand* pResponseCommand = NULL;
  responseFuture.setResponse(pResponseCommand);
  EXPECT_EQ(responseFuture.getRequestCommand().getCode(), 0);

  EXPECT_TRUE(responseFuture.getSyncResponseFlag());

  // m_bAsync = true  m_syncResponse
  ResponseFuture twoResponseFuture(13, 4, NULL, 1000, true);

  EXPECT_TRUE(twoResponseFuture.getASyncFlag());
  EXPECT_TRUE(twoResponseFuture.getSyncResponseFlag());
  EXPECT_FALSE(twoResponseFuture.getAsyncResponseFlag());

  twoResponseFuture.setResponse(pResponseCommand);
  EXPECT_TRUE(twoResponseFuture.getSyncResponseFlag());

  ResponseFuture threeSesponseFuture(13, 4, NULL, 1000);

  uint64_t millis = UtilAll::currentTimeMillis();
  RemotingCommand* remotingCommand = threeSesponseFuture.waitResponse(10);
  uint64_t useTime = UtilAll::currentTimeMillis() - millis;
  EXPECT_LT(useTime, 30);

  EXPECT_TRUE(responseFuture.getSyncResponseFlag());
  EXPECT_EQ(NULL, remotingCommand);
}

TEST(responseFuture, executeInvokeCallback) {
  //  executeInvokeCallback delete wrap
  MockAsyncCallbackWrap* wrap = new MockAsyncCallbackWrap(nullptr, nullptr);
  ResponseFuture responseFuture(13, 4, nullptr, 1000, false, wrap);

  RemotingCommand* pResponseCommand = new RemotingCommand();
  responseFuture.setResponse(pResponseCommand);
  responseFuture.executeInvokeCallback();
  EXPECT_EQ(NULL, responseFuture.getCommand());

  EXPECT_CALL(*wrap, operationComplete(_, _)).Times(1);
  pResponseCommand = new RemotingCommand();
  responseFuture.setResponse(pResponseCommand);
  responseFuture.setAsyncCallBackStatus(asyncCallBackStatus::asyncCallBackStatus_response);
  responseFuture.executeInvokeCallback();
  EXPECT_EQ(pResponseCommand->getCode(), 0);

  ResponseFuture twoResponseFuture(13, 4, nullptr, 1000, false, NULL);
  pResponseCommand = new RemotingCommand();
  twoResponseFuture.executeInvokeCallback();
  EXPECT_EQ(NULL, twoResponseFuture.getCommand());
}

TEST(responseFuture, executeInvokeCallbackException) {
  //  executeInvokeCallbackException delete wrap
  MockAsyncCallbackWrap* wrap = new MockAsyncCallbackWrap(nullptr, nullptr);

  ResponseFuture responseFuture(13, 4, nullptr, 1000, false, wrap);

  EXPECT_CALL(*wrap, onException()).Times(1);
  responseFuture.executeInvokeCallbackException();

  responseFuture.setAsyncCallBackStatus(asyncCallBackStatus::asyncCallBackStatus_timeout);
  responseFuture.executeInvokeCallbackException();

  ResponseFuture twoRresponseFuture(13, 4, nullptr, 1000, false, NULL);
  twoRresponseFuture.executeInvokeCallbackException();
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "responseFuture.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
