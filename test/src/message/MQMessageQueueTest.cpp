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

#include "MQMessageQueue.h"

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQMessageQueue;

TEST(messageQueue, init) {
  MQMessageQueue messageQueue;
  EXPECT_EQ(messageQueue.getBrokerName(), "");
  EXPECT_EQ(messageQueue.getTopic(), "");
  EXPECT_EQ(messageQueue.getQueueId(), -1);

  MQMessageQueue twoMessageQueue("testTopic", "testBroker", 1);
  EXPECT_EQ(twoMessageQueue.getBrokerName(), "testBroker");
  EXPECT_EQ(twoMessageQueue.getTopic(), "testTopic");
  EXPECT_EQ(twoMessageQueue.getQueueId(), 1);

  MQMessageQueue threeMessageQueue("threeTestTopic", "threeTestBroker", 2);
  MQMessageQueue frouMessageQueue(threeMessageQueue);
  EXPECT_EQ(frouMessageQueue.getBrokerName(), "threeTestBroker");
  EXPECT_EQ(frouMessageQueue.getTopic(), "threeTestTopic");
  EXPECT_EQ(frouMessageQueue.getQueueId(), 2);

  frouMessageQueue = twoMessageQueue;
  EXPECT_EQ(frouMessageQueue.getBrokerName(), "testBroker");
  EXPECT_EQ(frouMessageQueue.getTopic(), "testTopic");
  EXPECT_EQ(frouMessageQueue.getQueueId(), 1);

  frouMessageQueue.setBrokerName("frouTestBroker");
  frouMessageQueue.setTopic("frouTestTopic");
  frouMessageQueue.setQueueId(4);
  EXPECT_EQ(frouMessageQueue.getBrokerName(), "frouTestBroker");
  EXPECT_EQ(frouMessageQueue.getTopic(), "frouTestTopic");
  EXPECT_EQ(frouMessageQueue.getQueueId(), 4);
}

TEST(messageQueue, operators) {
  MQMessageQueue messageQueue;
  EXPECT_EQ(messageQueue, messageQueue);
  EXPECT_EQ(messageQueue.compareTo(messageQueue), 0);

  MQMessageQueue twoMessageQueue;
  EXPECT_EQ(messageQueue, twoMessageQueue);
  EXPECT_EQ(messageQueue.compareTo(twoMessageQueue), 0);

  twoMessageQueue.setTopic("testTopic");
  EXPECT_FALSE(messageQueue == twoMessageQueue);
  EXPECT_FALSE(messageQueue.compareTo(twoMessageQueue) == 0);

  twoMessageQueue.setQueueId(1);
  EXPECT_FALSE(messageQueue == twoMessageQueue);
  EXPECT_FALSE(messageQueue.compareTo(twoMessageQueue) == 0);

  twoMessageQueue.setBrokerName("testBroker");
  EXPECT_FALSE(messageQueue == twoMessageQueue);
  EXPECT_FALSE(messageQueue.compareTo(twoMessageQueue) == 0);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);

  testing::GTEST_FLAG(filter) = "messageQueue.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
