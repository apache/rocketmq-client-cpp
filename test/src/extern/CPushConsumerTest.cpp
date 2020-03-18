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

#include "string.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "CPushConsumer.h"

#include "ConsumeType.h"
#include "DefaultMQPushConsumer.h"
#include "MQMessageListener.h"
#include "SessionCredentials.h"

using std::string;

using ::testing::_;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Mock;
using testing::Return;

using rocketmq::DefaultMQPushConsumer;
using rocketmq::elogLevel;
using rocketmq::MessageListenerType;
using rocketmq::MessageModel;
using rocketmq::SessionCredentials;

class MockDefaultMQPushConsumer : public DefaultMQPushConsumer {
 public:
  MockDefaultMQPushConsumer(const string& groupname) : DefaultMQPushConsumer(groupname) {}

  MOCK_METHOD0(start, void());
  MOCK_METHOD0(shutdown, void());
  MOCK_METHOD2(setLogFileSizeAndNum, void(int, long));
  MOCK_METHOD1(setLogLevel, void(elogLevel));
};

TEST(cPushComsumer, infomock) {
  MockDefaultMQPushConsumer* pushComsumer = new MockDefaultMQPushConsumer("testGroup");
  CPushConsumer* consumer = (CPushConsumer*)pushComsumer;

  EXPECT_CALL(*pushComsumer, start()).Times(1);
  EXPECT_EQ(StartPushConsumer(consumer), OK);

  EXPECT_CALL(*pushComsumer, shutdown()).Times(1);
  EXPECT_EQ(ShutdownPushConsumer(consumer), OK);

  EXPECT_CALL(*pushComsumer, setLogFileSizeAndNum(1, 1)).Times(1);
  pushComsumer->setLogFileSizeAndNum(1, 1);
  EXPECT_EQ(SetPushConsumerLogFileNumAndSize(consumer, 1, 1), OK);

  // EXPECT_CALL(*pushComsumer,setLogLevel(_)).Times(1);
  EXPECT_EQ(SetPushConsumerLogLevel(consumer, E_LOG_LEVEL_FATAL), OK);

  Mock::AllowLeak(pushComsumer);
}

int MessageCallBackFunc(CPushConsumer* consumer, CMessageExt* msg) {
  return 0;
}

TEST(cPushComsumer, info) {
  CPushConsumer* cpushConsumer = CreatePushConsumer("testGroup");
  DefaultMQPushConsumer* mqPushConsumer = (DefaultMQPushConsumer*)cpushConsumer;

  EXPECT_TRUE(cpushConsumer != NULL);
  EXPECT_EQ(string(GetPushConsumerGroupID(cpushConsumer)), "testGroup");

  EXPECT_EQ(SetPushConsumerGroupID(cpushConsumer, "testGroupTwo"), OK);
  EXPECT_EQ(string(GetPushConsumerGroupID(cpushConsumer)), "testGroupTwo");

  EXPECT_EQ(SetPushConsumerNameServerAddress(cpushConsumer, "127.0.0.1:9876"), OK);
  EXPECT_EQ(mqPushConsumer->getNamesrvAddr(), "127.0.0.1:9876");

  EXPECT_EQ(SetPushConsumerNameServerDomain(cpushConsumer, "domain"), OK);
  EXPECT_EQ(mqPushConsumer->getNamesrvDomain(), "domain");

  EXPECT_EQ(Subscribe(cpushConsumer, "testTopic", "testSub"), OK);

  EXPECT_EQ(RegisterMessageCallbackOrderly(cpushConsumer, MessageCallBackFunc), OK);
  EXPECT_EQ(mqPushConsumer->getMessageListenerType(), MessageListenerType::messageListenerOrderly);

  EXPECT_EQ(RegisterMessageCallback(cpushConsumer, MessageCallBackFunc), OK);
  EXPECT_EQ(mqPushConsumer->getMessageListenerType(), MessageListenerType::messageListenerConcurrently);

  EXPECT_EQ(UnregisterMessageCallbackOrderly(cpushConsumer), OK);
  EXPECT_EQ(UnregisterMessageCallback(cpushConsumer), OK);

  EXPECT_EQ(SetPushConsumerThreadCount(cpushConsumer, 10), OK);
  EXPECT_EQ(mqPushConsumer->getConsumeThreadCount(), 10);

  EXPECT_EQ(SetPushConsumerMessageBatchMaxSize(cpushConsumer, 1024), OK);
  EXPECT_EQ(mqPushConsumer->getConsumeMessageBatchMaxSize(), 1024);

  EXPECT_EQ(SetPushConsumerInstanceName(cpushConsumer, "instance"), OK);
  EXPECT_EQ(mqPushConsumer->getInstanceName(), "instance");

  EXPECT_EQ(SetPushConsumerSessionCredentials(cpushConsumer, "accessKey", "secretKey", "channel"), OK);
  SessionCredentials sessionCredentials = mqPushConsumer->getSessionCredentials();
  EXPECT_EQ(sessionCredentials.getAccessKey(), "accessKey");

  EXPECT_EQ(SetPushConsumerMessageModel(cpushConsumer, BROADCASTING), OK);
  EXPECT_EQ(mqPushConsumer->getMessageModel(), MessageModel::BROADCASTING);

  EXPECT_EQ(SetPushConsumerMessageTrace(cpushConsumer, CLOSE), OK);
  EXPECT_EQ(mqPushConsumer->getMessageTrace(), false);
  Mock::AllowLeak(mqPushConsumer);
}

TEST(cPushComsumer, null) {
  CPushConsumer* cpushConsumer = CreatePushConsumer("testGroup");

  EXPECT_TRUE(CreatePushConsumer(NULL) == NULL);
  EXPECT_EQ(DestroyPushConsumer(NULL), NULL_POINTER);
  EXPECT_EQ(StartPushConsumer(NULL), NULL_POINTER);
  EXPECT_EQ(ShutdownPushConsumer(NULL), NULL_POINTER);
  EXPECT_EQ(SetPushConsumerGroupID(NULL, "testGroup"), NULL_POINTER);
  EXPECT_TRUE(GetPushConsumerGroupID(NULL) == NULL);
  EXPECT_EQ(SetPushConsumerNameServerAddress(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetPushConsumerNameServerDomain(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(Subscribe(NULL, NULL, NULL), NULL_POINTER);
  EXPECT_EQ(RegisterMessageCallbackOrderly(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(RegisterMessageCallbackOrderly(cpushConsumer, NULL), NULL_POINTER);
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
}
TEST(cPushComsumer, version) {
  CPushConsumer* pushConsumer = CreatePushConsumer("groupTestVersion");
  EXPECT_TRUE(pushConsumer != NULL);
  string version(ShowPushConsumerVersion(pushConsumer));
  EXPECT_GT(version.length(), 0);
}
int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(filter) = "cPushComsumer.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
