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
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "DefaultMQProducer.h"
#include "MQMessage.h"
#include "MQMessageQueue.h"
#include "MQSelector.h"
#include "SendResult.h"
#include "SessionCredentials.h"
#include "c/CMessage.h"
#include "c/CProducer.h"
#include "c/CSendResult.h"

using testing::_;
using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Mock;
using testing::Return;

using rocketmq::DefaultMQProducer;
using rocketmq::MessageQueueSelector;
using rocketmq::MQMessage;
using rocketmq::MQMessageQueue;
using rocketmq::SendCallback;
using rocketmq::SendResult;
using rocketmq::SendStatus;
using rocketmq::SessionCredentials;

class MockDefaultMQProducer : public DefaultMQProducer {
 public:
  MockDefaultMQProducer(const std::string& groupname) : DefaultMQProducer(groupname) {}

  MOCK_METHOD(void, start, (), (override));
  MOCK_METHOD(void, shutdown, (), (override));

  MOCK_METHOD(SendResult, send, (MQMessage&), (override));
  MOCK_METHOD(void, send, (MQMessage&, SendCallback*), (noexcept, override));
  MOCK_METHOD(SendResult, send, (MQMessage&, MessageQueueSelector*, void*, long), (override));
  MOCK_METHOD(void, sendOneway, (MQMessage&), (override));
};

void CSendSuccessCallbackFunc(CSendResult result) {}
void CSendExceptionCallbackFunc(CMQException e) {}

TEST(CProducerTest, SendMessageAsync) {
  MockDefaultMQProducer* mockProducer = new MockDefaultMQProducer("testGroup");
  CProducer* cProducer = (CProducer*)mockProducer;
  CMessage* msg = CreateMessage("");

  EXPECT_EQ(SendMessageAsync(NULL, NULL, NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SendMessageAsync(cProducer, NULL, NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SendMessageAsync(cProducer, msg, CSendSuccessCallbackFunc, NULL), NULL_POINTER);

  EXPECT_CALL(*mockProducer, send(_, _)).Times(1);
  EXPECT_EQ(SendMessageAsync(cProducer, msg, CSendSuccessCallbackFunc, CSendExceptionCallbackFunc), OK);
  Mock::AllowLeak(mockProducer);
  DestroyMessage(msg);
}

int QueueSelectorCallbackFunc(int size, CMessage* msg, void* arg) {
  return 0;
}

TEST(CProducerTest, SendMessageOrderly) {
  MockDefaultMQProducer* mockProducer = new MockDefaultMQProducer("testGroup");
  CProducer* cProducer = (CProducer*)mockProducer;
  CMessage* msg = (CMessage*)new MQMessage();
  MQMessageQueue messageQueue;

  EXPECT_EQ(SendMessageOrderly(NULL, NULL, NULL, msg, 1, NULL), NULL_POINTER);
  EXPECT_EQ(SendMessageOrderly(cProducer, NULL, NULL, msg, 1, NULL), NULL_POINTER);
  EXPECT_EQ(SendMessageOrderly(cProducer, msg, NULL, msg, 1, NULL), NULL_POINTER);
  EXPECT_EQ(SendMessageOrderly(cProducer, msg, QueueSelectorCallbackFunc, NULL, 1, NULL), NULL_POINTER);
  EXPECT_EQ(SendMessageOrderly(cProducer, msg, QueueSelectorCallbackFunc, msg, 1, NULL), NULL_POINTER);

  EXPECT_CALL(*mockProducer, send(_, _, _, _))
      .WillOnce(Return(SendResult(SendStatus::SEND_OK, "3", "offset1", messageQueue, 14)));
  // EXPECT_EQ(SendMessageOrderly(cProducer, msg, callback, msg, 1, result), OK);
  Mock::AllowLeak(mockProducer);
  DestroyMessage(msg);
  // free(result);
}

TEST(CProducerTest, SendOneway) {
  MockDefaultMQProducer* mockProducer = new MockDefaultMQProducer("testGroup");
  CProducer* cProducer = (CProducer*)mockProducer;
  CMessage* msg = (CMessage*)new MQMessage();

  EXPECT_EQ(SendMessageOneway(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SendMessageOneway(cProducer, NULL), NULL_POINTER);

  EXPECT_CALL(*mockProducer, sendOneway(_)).Times(1);
  EXPECT_EQ(SendMessageOneway(cProducer, msg), OK);

  Mock::AllowLeak(mockProducer);

  DestroyMessage(msg);
}

TEST(CProducerTest, SendMessageSync) {
  MockDefaultMQProducer* mockProducer = new MockDefaultMQProducer("testGroup");
  CProducer* cProducer = (CProducer*)mockProducer;

  MQMessage* mqMessage = new MQMessage();
  CMessage* msg = (CMessage*)mqMessage;
  CSendResult* result;
  MQMessageQueue messageQueue;
  EXPECT_EQ(SendMessageSync(NULL, NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SendMessageSync(cProducer, NULL, NULL), NULL_POINTER);

  EXPECT_EQ(SendMessageSync(cProducer, msg, NULL), NULL_POINTER);

  result = (CSendResult*)malloc(sizeof(CSendResult));

  EXPECT_CALL(*mockProducer, send(_))
      .Times(5)
      .WillOnce(Return(SendResult(SendStatus::SEND_FLUSH_DISK_TIMEOUT, "1", "offset1", messageQueue, 14)))
      .WillOnce(Return(SendResult(SendStatus::SEND_FLUSH_SLAVE_TIMEOUT, "2", "offset1", messageQueue, 14)))
      .WillOnce(Return(SendResult(SendStatus::SEND_SLAVE_NOT_AVAILABLE, "3", "offset1", messageQueue, 14)))
      .WillOnce(Return(SendResult(SendStatus::SEND_OK, "3", "offset1", messageQueue, 14)))
      .WillOnce(Return(SendResult((SendStatus)-1, "4", "offset1", messageQueue, 14)));

  EXPECT_EQ(SendMessageSync(cProducer, msg, result), OK);
  EXPECT_EQ(result->sendStatus, E_SEND_FLUSH_DISK_TIMEOUT);

  EXPECT_EQ(SendMessageSync(cProducer, msg, result), OK);
  EXPECT_EQ(result->sendStatus, E_SEND_FLUSH_SLAVE_TIMEOUT);

  EXPECT_EQ(SendMessageSync(cProducer, msg, result), OK);
  EXPECT_EQ(result->sendStatus, E_SEND_SLAVE_NOT_AVAILABLE);

  EXPECT_EQ(SendMessageSync(cProducer, msg, result), OK);
  EXPECT_EQ(result->sendStatus, E_SEND_OK);

  EXPECT_EQ(SendMessageSync(cProducer, msg, result), OK);
  EXPECT_EQ(result->sendStatus, E_SEND_OK);

  Mock::AllowLeak(mockProducer);

  DestroyMessage(msg);
  free(result);
}

TEST(CProducerTest, InfoMock) {
  MockDefaultMQProducer* mockProducer = new MockDefaultMQProducer("testGroup");
  CProducer* cProducer = (CProducer*)mockProducer;

  EXPECT_CALL(*mockProducer, start()).Times(1);
  EXPECT_EQ(StartProducer(cProducer), OK);

  EXPECT_CALL(*mockProducer, shutdown()).Times(1);
  EXPECT_EQ(ShutdownProducer(cProducer), OK);

  Mock::AllowLeak(mockProducer);
}

TEST(CProducerTest, Info) {
  CProducer* cProducer = CreateProducer("groupTest");
  DefaultMQProducer* defaultMQProducer = (DefaultMQProducer*)cProducer;
  EXPECT_TRUE(cProducer != NULL);
  EXPECT_EQ(defaultMQProducer->group_name(), "groupTest");

  EXPECT_EQ(SetProducerNameServerAddress(cProducer, "127.0.0.1:9876"), OK);
  EXPECT_EQ(defaultMQProducer->namesrv_addr(), "127.0.0.1:9876");

  EXPECT_EQ(SetProducerGroupName(cProducer, "testGroup"), OK);
  EXPECT_EQ(defaultMQProducer->group_name(), "testGroup");

  EXPECT_EQ(SetProducerInstanceName(cProducer, "instance"), OK);
  EXPECT_EQ(defaultMQProducer->instance_name(), "instance");

  EXPECT_EQ(SetProducerSendMsgTimeout(cProducer, 1), OK);
  EXPECT_EQ(defaultMQProducer->send_msg_timeout(), 1);

  EXPECT_EQ(SetProducerMaxMessageSize(cProducer, 2), OK);
  EXPECT_EQ(defaultMQProducer->max_message_size(), 2);

  EXPECT_EQ(SetProducerCompressLevel(cProducer, 1), OK);
  EXPECT_EQ(defaultMQProducer->compress_level(), 1);

  EXPECT_EQ(SetProducerSessionCredentials(NULL, NULL, NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetProducerSessionCredentials(cProducer, "accessKey", "secretKey", "channel"), OK);
  // SessionCredentials sessionCredentials = defaultMQProducer->getSessionCredentials();
  // EXPECT_EQ(sessionCredentials.getAccessKey(), "accessKey");

  Mock::AllowLeak(defaultMQProducer);
}

TEST(CProducerTest, CheckNull) {
  EXPECT_TRUE(CreateProducer(NULL) == NULL);
  EXPECT_EQ(StartProducer(NULL), NULL_POINTER);
  EXPECT_EQ(ShutdownProducer(NULL), NULL_POINTER);
  EXPECT_EQ(SetProducerNameServerAddress(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetProducerGroupName(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetProducerInstanceName(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetProducerSessionCredentials(NULL, NULL, NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetProducerLogLevel(NULL, E_LOG_LEVEL_FATAL), NULL_POINTER);
  EXPECT_EQ(SetProducerSendMsgTimeout(NULL, 1), NULL_POINTER);
  EXPECT_EQ(SetProducerCompressLevel(NULL, 1), NULL_POINTER);
  EXPECT_EQ(SetProducerMaxMessageSize(NULL, 2), NULL_POINTER);
  EXPECT_EQ(SetProducerLogLevel(NULL, E_LOG_LEVEL_FATAL), NULL_POINTER);
  EXPECT_EQ(DestroyProducer(NULL), NULL_POINTER);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "CProducerTest.*";
  return RUN_ALL_TESTS();
}
