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

#include "ConsumeType.h"
#include "DefaultMQPushConsumer.h"
#include "MQMessageListener.h"
#include "SessionCredentials.h"
#include "c/CPushConsumer.h"

using testing::_;
using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Mock;
using testing::Return;

using rocketmq::DefaultMQPushConsumer;
using rocketmq::MessageListenerType;
using rocketmq::MessageModel;
using rocketmq::SessionCredentials;

class MockDefaultMQPushConsumer : public DefaultMQPushConsumer {
 public:
  MockDefaultMQPushConsumer(const std::string& groupname) : DefaultMQPushConsumer(groupname) {}

  MOCK_METHOD(void, start, (), (override));
  MOCK_METHOD(void, shutdown, (), (override));
};

TEST(CPushComsumerTest, InfoMock) {
  MockDefaultMQPushConsumer* pushComsumer = new MockDefaultMQPushConsumer("testGroup");
  CPushConsumer* consumer = reinterpret_cast<CPushConsumer*>(static_cast<DefaultMQPushConsumer*>(pushComsumer));

  EXPECT_CALL(*pushComsumer, start()).Times(1);
  EXPECT_EQ(StartPushConsumer(consumer), OK);

  EXPECT_CALL(*pushComsumer, shutdown()).Times(1);
  EXPECT_EQ(ShutdownPushConsumer(consumer), OK);

  delete pushComsumer;
}

int MessageCallBackFunc(CPushConsumer* consumer, CMessageExt* msg) {
  return 0;
}

TEST(CPushComsumerTest, Info) {
  CPushConsumer* cPushConsumer = CreatePushConsumer("testGroup");
  DefaultMQPushConsumer* mqPushConsumer = reinterpret_cast<DefaultMQPushConsumer*>(cPushConsumer);

  EXPECT_TRUE(cPushConsumer != NULL);
  EXPECT_STREQ(GetPushConsumerGroupID(cPushConsumer), "testGroup");

  EXPECT_EQ(SetPushConsumerGroupID(cPushConsumer, "testGroupTwo"), OK);
  EXPECT_STREQ(GetPushConsumerGroupID(cPushConsumer), "testGroupTwo");

  EXPECT_EQ(SetPushConsumerNameServerAddress(cPushConsumer, "127.0.0.1:9876"), OK);
  EXPECT_EQ(mqPushConsumer->namesrv_addr(), "127.0.0.1:9876");

  EXPECT_EQ(Subscribe(cPushConsumer, "testTopic", "testSub"), OK);

  EXPECT_EQ(RegisterMessageCallbackOrderly(cPushConsumer, MessageCallBackFunc), OK);
  EXPECT_EQ(mqPushConsumer->getMessageListener()->getMessageListenerType(),
            MessageListenerType::messageListenerOrderly);
  EXPECT_EQ(UnregisterMessageCallbackOrderly(cPushConsumer), OK);

  EXPECT_EQ(RegisterMessageCallback(cPushConsumer, MessageCallBackFunc), OK);
  EXPECT_EQ(mqPushConsumer->getMessageListener()->getMessageListenerType(),
            MessageListenerType::messageListenerConcurrently);
  EXPECT_EQ(UnregisterMessageCallback(cPushConsumer), OK);

  EXPECT_EQ(SetPushConsumerThreadCount(cPushConsumer, 10), OK);
  EXPECT_EQ(mqPushConsumer->consume_thread_nums(), 10);

  EXPECT_EQ(SetPushConsumerMessageBatchMaxSize(cPushConsumer, 1024), OK);
  EXPECT_EQ(mqPushConsumer->consume_message_batch_max_size(), 1024);

  EXPECT_EQ(SetPushConsumerInstanceName(cPushConsumer, "instance"), OK);
  EXPECT_EQ(mqPushConsumer->instance_name(), "instance");

  EXPECT_EQ(SetPushConsumerSessionCredentials(cPushConsumer, "accessKey", "secretKey", "channel"), OK);
  // SessionCredentials sessionCredentials = mqPushConsumer->getSessionCredentials();
  // EXPECT_EQ(sessionCredentials.getAccessKey(), "accessKey");

  EXPECT_EQ(SetPushConsumerMessageModel(cPushConsumer, BROADCASTING), OK);
  EXPECT_EQ(mqPushConsumer->message_model(), MessageModel::BROADCASTING);

  DestroyPushConsumer(cPushConsumer);
}

TEST(CPushComsumerTest, CheckNull) {
  CPushConsumer* cPushConsumer = CreatePushConsumer("testGroup");

  EXPECT_TRUE(CreatePushConsumer(NULL) == NULL);
  EXPECT_EQ(DestroyPushConsumer(NULL), NULL_POINTER);
  EXPECT_EQ(StartPushConsumer(NULL), NULL_POINTER);
  EXPECT_EQ(ShutdownPushConsumer(NULL), NULL_POINTER);
  EXPECT_EQ(SetPushConsumerGroupID(NULL, "testGroup"), NULL_POINTER);
  EXPECT_TRUE(GetPushConsumerGroupID(NULL) == NULL);
  EXPECT_EQ(SetPushConsumerNameServerAddress(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(Subscribe(NULL, NULL, NULL), NULL_POINTER);
  EXPECT_EQ(RegisterMessageCallbackOrderly(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(RegisterMessageCallbackOrderly(cPushConsumer, NULL), NULL_POINTER);
  EXPECT_EQ(RegisterMessageCallback(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(UnregisterMessageCallbackOrderly(NULL), NULL_POINTER);
  EXPECT_EQ(UnregisterMessageCallback(NULL), NULL_POINTER);
  EXPECT_EQ(SetPushConsumerThreadCount(NULL, 0), NULL_POINTER);
  EXPECT_EQ(SetPushConsumerMessageBatchMaxSize(NULL, 0), NULL_POINTER);
  EXPECT_EQ(SetPushConsumerInstanceName(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetPushConsumerSessionCredentials(NULL, NULL, NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetPushConsumerLogPath(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetPushConsumerLogFileNumAndSize(NULL, 1, 1), NULL_POINTER);
  EXPECT_EQ(SetPushConsumerLogLevel(NULL, E_LOG_LEVEL_LEVEL_NUM), NULL_POINTER);
  EXPECT_EQ(SetPushConsumerMessageModel(NULL, BROADCASTING), NULL_POINTER);

  DestroyPushConsumer(cPushConsumer);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "CPushComsumerTest.*";
  return RUN_ALL_TESTS();
}
