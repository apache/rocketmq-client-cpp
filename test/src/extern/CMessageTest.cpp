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

#include "MQMessage.h"
#include "c/CCommon.h"
#include "c/CMessage.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQMessage;

TEST(CMessagesTest, CheckProperties) {
  CMessage* message = CreateMessage(NULL);
  MQMessage* mqMessage = (MQMessage*)message;
  EXPECT_EQ(mqMessage->topic(), "");

  SetMessageTopic(message, "testTopic");
  EXPECT_EQ(mqMessage->topic(), "testTopic");

  SetMessageTags(message, "testTags");
  EXPECT_EQ(mqMessage->tags(), "testTags");

  SetMessageKeys(message, "testKeys");
  EXPECT_EQ(mqMessage->keys(), "testKeys");

  SetMessageBody(message, "testBody");
  EXPECT_EQ(mqMessage->body(), "testBody");

  SetByteMessageBody(message, "testBody", 5);
  EXPECT_EQ(mqMessage->body(), "testB");

  SetMessageProperty(message, "testProperty", "testValue");
  EXPECT_EQ(mqMessage->getProperty("testProperty"), "testValue");

  SetDelayTimeLevel(message, 1);
  EXPECT_EQ(mqMessage->delay_time_level(), 1);

  EXPECT_EQ(DestroyMessage(message), OK);

  message = CreateMessage("testTopic");
  mqMessage = (MQMessage*)message;
  EXPECT_EQ(mqMessage->topic(), "testTopic");

  EXPECT_EQ(DestroyMessage(message), OK);
}

TEST(CMessagesTest, CheckNull) {
  EXPECT_EQ(SetMessageTopic(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetMessageTags(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetMessageKeys(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetMessageBody(NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetByteMessageBody(NULL, NULL, 0), NULL_POINTER);
  EXPECT_EQ(SetMessageProperty(NULL, NULL, NULL), NULL_POINTER);
  EXPECT_EQ(SetDelayTimeLevel(NULL, 0), NULL_POINTER);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(filter) = "CMessagesTest.*";
  return RUN_ALL_TESTS();
}
