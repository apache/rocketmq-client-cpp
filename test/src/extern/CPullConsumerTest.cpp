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

#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "CCommon.h"
#include "CPullConsumer.h"

#include "DefaultMQPullConsumer.h"
#include "MQClient.h"
#include "MQMessageExt.h"
#include "MQMessageQueue.h"
#include "PullResult.h"
#include "SessionCredentials.h"

using std::string;
using std::vector;

using ::testing::_;
using testing::Expectation;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;
using testing::SetArgReferee;

using rocketmq::DefaultMQPullConsumer;
using rocketmq::elogLevel;
using rocketmq::MessageModel;
using rocketmq::MQMessageExt;
using rocketmq::MQMessageQueue;
using rocketmq::PullResult;
using rocketmq::PullStatus;
using rocketmq::SessionCredentials;

class MockDefaultMQPullConsumer : public DefaultMQPullConsumer {
 public:
  MockDefaultMQPullConsumer(const string& groupname) : DefaultMQPullConsumer(groupname) {}
  MOCK_METHOD0(start, void());
  MOCK_METHOD0(shutdown, void());
  MOCK_METHOD2(setLogFileSizeAndNum, void(int, long));
  MOCK_METHOD1(setLogLevel, void(elogLevel));
  MOCK_METHOD2(fetchSubscribeMessageQueues, void(const string&, vector<MQMessageQueue>&));
  MOCK_METHOD4(pull, PullResult(const MQMessageQueue&, const string&, int64, int));
};

TEST(cpullConsumer, pull) {
  MockDefaultMQPullConsumer* mqPullConsumer = new MockDefaultMQPullConsumer("groudId");
  CPullConsumer* pullConsumer = (CPullConsumer*)mqPullConsumer;

  CMessageQueue cMessageQueue;
  strncpy(cMessageQueue.topic, "testTopic", 8);
  strncpy(cMessageQueue.brokerName, "testBroker", 9);
  cMessageQueue.queueId = 1;

  PullResult timeOutPullResult(PullStatus::BROKER_TIMEOUT, 1, 2, 3);

  PullResult noNewMsgPullResult(PullStatus::NO_NEW_MSG, 1, 2, 3);

  PullResult noMatchedMsgPullResult(PullStatus::NO_MATCHED_MSG, 1, 2, 3);

  PullResult offsetIllegalPullResult(PullStatus::OFFSET_ILLEGAL, 1, 2, 3);

  PullResult defaultPullResult((PullStatus)-1, 1, 2, 3);

  vector<MQMessageExt> src;
  for (int i = 0; i < 5; i++) {
    MQMessageExt ext;
    src.push_back(ext);
  }

  PullResult foundPullResult(PullStatus::FOUND, 1, 2, 3, src);
  EXPECT_CALL(*mqPullConsumer, pull(_, _, _, _))
      .WillOnce(Return(timeOutPullResult))
      .WillOnce(Return(noNewMsgPullResult))
      .WillOnce(Return(noMatchedMsgPullResult))
      .WillOnce(Return(offsetIllegalPullResult))
      .WillOnce(Return(defaultPullResult))
      //.WillOnce(Return(timeOutPullResult)) //will not called
      .WillOnce(Return(foundPullResult));
  CPullResult timeOutcPullResult = Pull(pullConsumer, &cMessageQueue, "123123", 0, 0);
  EXPECT_EQ(timeOutcPullResult.pullStatus, E_BROKER_TIMEOUT);
  CPullResult noNewMsgcPullResult = Pull(pullConsumer, &cMessageQueue, "123123", 0, 0);
  EXPECT_EQ(noNewMsgcPullResult.pullStatus, E_NO_NEW_MSG);

  CPullResult noMatchedMsgcPullResult = Pull(pullConsumer, &cMessageQueue, "123123", 0, 0);
  EXPECT_EQ(noMatchedMsgcPullResult.pullStatus, E_NO_MATCHED_MSG);

  CPullResult offsetIllegalcPullResult = Pull(pullConsumer, &cMessageQueue, "123123", 0, 0);
  EXPECT_EQ(offsetIllegalcPullResult.pullStatus, E_OFFSET_ILLEGAL);

  CPullResult defaultcPullResult = Pull(pullConsumer, &cMessageQueue, "123123", 0, 0);
  EXPECT_EQ(defaultcPullResult.pullStatus, E_NO_NEW_MSG);
  CPullResult exceptionPullResult = Pull(pullConsumer, &cMessageQueue, NULL, 0, 0);
  EXPECT_EQ(exceptionPullResult.pullStatus, E_BROKER_TIMEOUT);
  CPullResult foundcPullResult = Pull(pullConsumer, &cMessageQueue, "123123", 0, 0);
  EXPECT_EQ(foundcPullResult.pullStatus, E_FOUND);

  delete mqPullConsumer;
}

TEST(cpullConsumer, infoMock) {
  MockDefaultMQPullConsumer* mqPullConsumer = new MockDefaultMQPullConsumer("groudId");
  CPullConsumer* pullConsumer = (CPullConsumer*)mqPullConsumer;

  Expectation exp = EXPECT_CALL(*mqPullConsumer, start()).Times(1);
  EXPECT_EQ(StartPullConsumer(pullConsumer), OK);

  EXPECT_CALL(*mqPullConsumer, shutdown()).Times(1);
  EXPECT_EQ(ShutdownPullConsumer(pullConsumer), OK);

  // EXPECT_CALL(*mqPullConsumer,setLogFileSizeAndNum(_,_)).Times(1);
  EXPECT_EQ(SetPullConsumerLogFileNumAndSize(pullConsumer, 1, 2), OK);

  // EXPECT_CALL(*mqPullConsumer,setLogLevel(_)).Times(1);
  EXPECT_EQ(SetPullConsumerLogLevel(pullConsumer, E_LOG_LEVEL_INFO), OK);

  std::vector<MQMessageQueue> fullMQ;
  for (int i = 0; i < 5; i++) {
    MQMessageQueue queue("testTopic", "testsBroker", i);
    fullMQ.push_back(queue);
  }

  EXPECT_CALL(*mqPullConsumer, fetchSubscribeMessageQueues(_, _)).Times(1).WillOnce(SetArgReferee<1>(fullMQ));
  CMessageQueue* mqs = NULL;
  int size = 0;
  FetchSubscriptionMessageQueues(pullConsumer, "testTopic", &mqs, &size);
  EXPECT_EQ(size, 5);

  delete mqPullConsumer;
}

TEST(cpullConsumer, init) {
  CPullConsumer* pullConsumer = CreatePullConsumer("testGroupId");
  DefaultMQPullConsumer* defaultMQPullConsumer = (DefaultMQPullConsumer*)pullConsumer;
  EXPECT_FALSE(pullConsumer == NULL);

  EXPECT_EQ(SetPullConsumerGroupID(pullConsumer, "groupId"), OK);
  EXPECT_EQ(GetPullConsumerGroupID(pullConsumer), defaultMQPullConsumer->getGroupName().c_str());

  EXPECT_EQ(SetPullConsumerNameServerAddress(pullConsumer, "127.0.0.1:10091"), OK);
  EXPECT_EQ(defaultMQPullConsumer->getNamesrvAddr(), "127.0.0.1:10091");

  EXPECT_EQ(SetPullConsumerNameServerDomain(pullConsumer, "domain"), OK);
  EXPECT_EQ(defaultMQPullConsumer->getNamesrvDomain(), "domain");

  EXPECT_EQ(SetPullConsumerSessionCredentials(pullConsumer, "accessKey", "secretKey", "channel"), OK);
  SessionCredentials sessionCredentials = defaultMQPullConsumer->getSessionCredentials();
  EXPECT_EQ(sessionCredentials.getAccessKey(), "accessKey");

  EXPECT_EQ(SetPullConsumerLogPath(pullConsumer, NULL), OK);

  // EXPECT_EQ(SetPullConsumerLogFileNumAndSize(pullConsumer,NULL,NULL),NULL_POINTER);
  EXPECT_EQ(SetPullConsumerLogLevel(pullConsumer, E_LOG_LEVEL_DEBUG), OK);
}

TEST(cpullConsumer, null) {
  CPullConsumer* pullConsumer = CreatePullConsumer("testGroupId");
  DefaultMQPullConsumer* defaultMQPullConsumer = (DefaultMQPullConsumer*)pullConsumer;
  EXPECT_FALSE(pullConsumer == NULL);

  EXPECT_EQ(SetPullConsumerGroupID(pullConsumer, "groupId"), OK);
  EXPECT_EQ(GetPullConsumerGroupID(pullConsumer), defaultMQPullConsumer->getGroupName().c_str());

  EXPECT_EQ(SetPullConsumerNameServerAddress(pullConsumer, "127.0.0.1:10091"), OK);
  EXPECT_EQ(defaultMQPullConsumer->getNamesrvAddr(), "127.0.0.1:10091");

  EXPECT_EQ(SetPullConsumerNameServerDomain(pullConsumer, "domain"), OK);
  EXPECT_EQ(defaultMQPullConsumer->getNamesrvDomain(), "domain");

  EXPECT_EQ(SetPullConsumerSessionCredentials(pullConsumer, "accessKey", "secretKey", "channel"), OK);
  SessionCredentials sessionCredentials = defaultMQPullConsumer->getSessionCredentials();
  EXPECT_EQ(sessionCredentials.getAccessKey(), "accessKey");

  EXPECT_EQ(SetPullConsumerLogPath(pullConsumer, NULL), OK);
  // EXPECT_EQ(SetPullConsumerLogFileNumAndSize(pullConsumer,NULL,NULL),NULL_POINTER);
  EXPECT_EQ(SetPullConsumerLogLevel(pullConsumer, E_LOG_LEVEL_DEBUG), OK);
  EXPECT_EQ(DestroyPullConsumer(pullConsumer), OK);
  EXPECT_EQ(StartPullConsumer(NULL), NULL_POINTER);
  EXPECT_EQ(ShutdownPullConsumer(NULL), NULL_POINTER);
}
TEST(cpullConsumer, version) {
  CPullConsumer* pullConsumer = CreatePullConsumer("groupTestVersion");
  EXPECT_TRUE(pullConsumer != NULL);
  string version(ShowPullConsumerVersion(pullConsumer));
  EXPECT_GT(version.length(), 0);
}
int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "cpullConsumer.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
