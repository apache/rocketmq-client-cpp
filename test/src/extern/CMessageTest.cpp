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

#include <cstring>
#include <string>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "CCommon.h"
#include "CMessage.h"
#include "MQMessage.h"

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQMessage;

TEST(cmessages, originMessage) {
  CMessage* message = CreateMessage(NULL);
  EXPECT_STREQ(GetOriginMessageTopic(message), "");

  SetMessageTopic(message, "testTopic");
  EXPECT_STREQ(GetOriginMessageTopic(message), "testTopic");

  SetMessageTags(message, "testTags");
  EXPECT_STREQ(GetOriginMessageTags(message), "testTags");

  SetMessageKeys(message, "testKeys");
  EXPECT_STREQ(GetOriginMessageKeys(message), "testKeys");

  std::string body("test_body");
  body.append(3, '\0');
  SetByteMessageBody(message, body.c_str(), body.length());
  std::string retrieved_body(GetOriginMessageBody(message), GetOriginMessageBodyLength(message));
  EXPECT_TRUE(body == retrieved_body);

  SetMessageProperty(message, "testKey", "testValue");
  EXPECT_STREQ(GetOriginMessageProperty(message, "testKey"), "testValue");

  SetDelayTimeLevel(message, 1);
  EXPECT_EQ(GetOriginDelayTimeLevel(message), 1);

  EXPECT_EQ(DestroyMessage(message), OK);

  CMessage* message2 = CreateMessage("testTwoTopic");
  EXPECT_STREQ(GetOriginMessageTopic(message2), "testTwoTopic");

  EXPECT_EQ(DestroyMessage(message2), OK);
}

TEST(cmessages, info) {
  CMessage* message = CreateMessage(NULL);
  MQMessage* mqMessage = (MQMessage*)message;
  EXPECT_EQ(mqMessage->getTopic(), "");

  SetMessageTopic(message, "testTopic");
  EXPECT_EQ(mqMessage->getTopic(), "testTopic");

  SetMessageTags(message, "testTags");
  EXPECT_EQ(mqMessage->getTags(), "testTags");

  SetMessageKeys(message, "testKeys");
  EXPECT_EQ(mqMessage->getKeys(), "testKeys");

  SetMessageBody(message, "testBody");
  EXPECT_EQ(mqMessage->getBody(), "testBody");

  SetByteMessageBody(message, "testBody", 5);
  EXPECT_EQ(mqMessage->getBody(), "testB");

  SetMessageProperty(message, "testKey", "testValue");
  EXPECT_EQ(mqMessage->getProperty("testKey"), "testValue");

  SetDelayTimeLevel(message, 1);
  EXPECT_EQ(mqMessage->getDelayTimeLevel(), 1);

  EXPECT_EQ(DestroyMessage(message), OK);

  CMessage* twomessage = CreateMessage("testTwoTopic");
  MQMessage* twoMqMessage = (MQMessage*)twomessage;
  EXPECT_EQ(twoMqMessage->getTopic(), "testTwoTopic");

  EXPECT_EQ(DestroyMessage(twomessage), OK);
}

TEST(cmessages, null) {
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

  // testing::GTEST_FLAG(filter) = "cmessages.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
