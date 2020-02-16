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

#include <map>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MQMessageQueue.h"
#include "TopicPublishInfo.h"

using namespace std;
using namespace rocketmq;
using rocketmq::MQMessageQueue;
using rocketmq::TopicPublishInfo;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

TEST(TopicPublishInfoTest, testAll) {
  TopicPublishInfo* info = new TopicPublishInfo();

  MQMessageQueue mqA("TestTopicA", "BrokerA", 0);
  MQMessageQueue mqB("TestTopicA", "BrokerB", 0);
  int index = -1;
  EXPECT_EQ(info->getWhichQueue(), 0);
  EXPECT_FALSE(info->ok());
  EXPECT_EQ(info->selectOneMessageQueue(mqA, index), MQMessageQueue());
  EXPECT_EQ(info->selectOneActiveMessageQueue(mqA, index), MQMessageQueue());
  info->updateMessageQueueList(mqA);
  info->updateMessageQueueList(mqB);
  EXPECT_TRUE(info->ok());
  EXPECT_EQ(info->getMessageQueueList().size(), 2);

  EXPECT_EQ(info->selectOneMessageQueue(mqA, index), MQMessageQueue());
  EXPECT_EQ(info->selectOneActiveMessageQueue(mqA, index), MQMessageQueue());
  index = 0;
  MQMessageQueue mqSelect1 = info->selectOneMessageQueue(MQMessageQueue(), index);
  EXPECT_EQ(index, 0);
  EXPECT_EQ(mqSelect1, mqA);
  EXPECT_EQ(info->getWhichQueue(), 1);
  MQMessageQueue mqSelect2 = info->selectOneMessageQueue(mqSelect1, index);
  EXPECT_EQ(index, 1);
  EXPECT_EQ(mqSelect2, mqB);
  EXPECT_EQ(info->getWhichQueue(), 3);
  index = 0;
  MQMessageQueue mqActiveSelect1 = info->selectOneActiveMessageQueue(MQMessageQueue(), index);
  EXPECT_EQ(index, 0);
  EXPECT_EQ(mqActiveSelect1, mqA);
  MQMessageQueue mqActiveSelect2 = info->selectOneActiveMessageQueue(mqActiveSelect1, index);
  EXPECT_EQ(index, 1);
  EXPECT_EQ(mqActiveSelect2, mqB);
  EXPECT_EQ(info->getWhichQueue(), 6);
  info->updateNonServiceMessageQueue(mqA, 1000);
  info->updateNonServiceMessageQueue(mqA, 1000);
  index = 0;
  MQMessageQueue mqActiveSelect3 = info->selectOneActiveMessageQueue(mqActiveSelect1, index);
  EXPECT_EQ(index, 1);
  EXPECT_EQ(mqActiveSelect3, mqB);
  MQMessageQueue mqActiveSelect4 = info->selectOneActiveMessageQueue(mqActiveSelect2, index);
  EXPECT_EQ(index, 1);
  EXPECT_EQ(mqActiveSelect4, mqA);
  info->updateNonServiceMessageQueue(mqB, 1000);
  index = 0;
  MQMessageQueue mqSelect3 = info->selectOneMessageQueue(MQMessageQueue(), index);
  EXPECT_EQ(index, 0);
  EXPECT_EQ(mqSelect3, mqA);
  index = 0;
  MQMessageQueue mqActiveSelect5 = info->selectOneActiveMessageQueue(MQMessageQueue(), index);
  EXPECT_EQ(index, 1);
  EXPECT_EQ(mqActiveSelect5, mqA);
  index = 0;
  MQMessageQueue mqActiveSelect6 = info->selectOneActiveMessageQueue(mqB, index);
  EXPECT_EQ(index, 0);
  EXPECT_EQ(mqActiveSelect6, mqA);
  info->updateMessageQueueList(mqSelect3);
  info->resumeNonServiceMessageQueueList();
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
