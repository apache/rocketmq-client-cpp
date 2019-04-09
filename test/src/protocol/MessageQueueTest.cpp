/*"
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

#include "MessageQueue.h"

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MessageQueue;

TEST(messageExt, init) {
  MessageQueue messageQueue;
  EXPECT_EQ(messageQueue.getQueueId(), -1);

  MessageQueue twoMessageQueue("testTopic", "testBroker", 1);
  EXPECT_EQ(twoMessageQueue.getQueueId(), 1);
  EXPECT_EQ(twoMessageQueue.getTopic(), "testTopic");
  EXPECT_EQ(twoMessageQueue.getBrokerName(), "testBroker");

  MessageQueue threeMessageQueue(twoMessageQueue);
  EXPECT_EQ(threeMessageQueue.getQueueId(), 1);
  EXPECT_EQ(threeMessageQueue.getTopic(), "testTopic");
  EXPECT_EQ(threeMessageQueue.getBrokerName(), "testBroker");

  Json::Value outJson = twoMessageQueue.toJson();
  EXPECT_EQ(outJson["queueId"], 1);
  EXPECT_EQ(outJson["topic"], "testTopic");
  EXPECT_EQ(outJson["brokerName"], "testBroker");

  MessageQueue fiveMessageQueue = threeMessageQueue;
  EXPECT_EQ(fiveMessageQueue.getQueueId(), 1);
  EXPECT_EQ(fiveMessageQueue.getTopic(), "testTopic");
  EXPECT_EQ(fiveMessageQueue.getBrokerName(), "testBroker");
}

TEST(messageExt, info) {
  MessageQueue messageQueue;

  messageQueue.setQueueId(11);
  EXPECT_EQ(messageQueue.getQueueId(), 11);

  messageQueue.setTopic("testTopic");
  EXPECT_EQ(messageQueue.getTopic(), "testTopic");

  messageQueue.setBrokerName("testBroker");
  EXPECT_EQ(messageQueue.getBrokerName(), "testBroker");
}

TEST(messageExt, operators) {
  MessageQueue messageQueue;
  EXPECT_TRUE(messageQueue == messageQueue);

  MessageQueue twoMessageQueue;
  EXPECT_TRUE(messageQueue == twoMessageQueue);

  twoMessageQueue.setTopic("testTopic");
  EXPECT_FALSE(messageQueue == twoMessageQueue);

  messageQueue.setQueueId(11);
  EXPECT_FALSE(messageQueue == twoMessageQueue);

  messageQueue.setBrokerName("testBroker");
  EXPECT_FALSE(messageQueue == twoMessageQueue);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "messageExt.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
