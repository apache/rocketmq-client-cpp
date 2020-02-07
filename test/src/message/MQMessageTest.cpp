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
#include <stdio.h>

#include <map>
#include <string>

#include "MQMessage.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace std;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQMessage;

TEST(message, Init) {
  MQMessage messageOne;
  EXPECT_EQ(messageOne.getTopic(), "");
  EXPECT_EQ(messageOne.getBody(), "");
  EXPECT_EQ(messageOne.getTags(), "");
  EXPECT_EQ(messageOne.getFlag(), 0);

  MQMessage messageTwo("test", "testBody");
  EXPECT_EQ(messageTwo.getTopic(), "test");
  EXPECT_EQ(messageTwo.getBody(), "testBody");
  EXPECT_EQ(messageTwo.getTags(), "");
  EXPECT_EQ(messageTwo.getFlag(), 0);

  MQMessage messageThree("test", "tagTest", "testBody");
  EXPECT_EQ(messageThree.getTopic(), "test");
  EXPECT_EQ(messageThree.getBody(), "testBody");
  EXPECT_EQ(messageThree.getTags(), "tagTest");
  EXPECT_EQ(messageThree.getFlag(), 0);

  MQMessage messageFour("test", "tagTest", "testKey", "testBody");
  EXPECT_EQ(messageFour.getTopic(), "test");
  EXPECT_EQ(messageFour.getBody(), "testBody");
  EXPECT_EQ(messageFour.getTags(), "tagTest");
  EXPECT_EQ(messageFour.getKeys(), "testKey");
  EXPECT_EQ(messageFour.getFlag(), 0);

  MQMessage messageFive("test", "tagTest", "testKey", 1, "testBody", 2);
  EXPECT_EQ(messageFive.getTopic(), "test");
  EXPECT_EQ(messageFive.getBody(), "testBody");
  EXPECT_EQ(messageFive.getTags(), "tagTest");
  EXPECT_EQ(messageFive.getKeys(), "testKey");
  EXPECT_EQ(messageFive.getFlag(), 1);

  MQMessage messageSix(messageFive);
  EXPECT_EQ(messageSix.getTopic(), "test");
  EXPECT_EQ(messageSix.getBody(), "testBody");
  EXPECT_EQ(messageSix.getTags(), "tagTest");
  EXPECT_EQ(messageSix.getKeys(), "testKey");
  EXPECT_EQ(messageSix.getFlag(), 1);

  MQMessage messageSeven = messageSix;
  EXPECT_EQ(messageSeven.getTopic(), "test");
  EXPECT_EQ(messageSeven.getBody(), "testBody");
  EXPECT_EQ(messageSeven.getTags(), "tagTest");
  EXPECT_EQ(messageSeven.getKeys(), "testKey");
  EXPECT_EQ(messageSeven.getFlag(), 1);
}

TEST(message, info) {
  MQMessage message;

  EXPECT_EQ(message.getTopic(), "");
  message.setTopic("testTopic");
  EXPECT_EQ(message.getTopic(), "testTopic");
  string topic = "testTopic";
  const char* ctopic = topic.c_str();
  message.setTopic(ctopic, 5);
  EXPECT_EQ(message.getTopic(), "testT");

  EXPECT_EQ(message.getBody(), "");
  message.setBody("testBody");
  EXPECT_EQ(message.getBody(), "testBody");

  string body = "testBody";
  const char* b = body.c_str();
  message.setBody(b, 5);
  EXPECT_EQ(message.getBody(), "testB");

  string tags(message.getTags());
  EXPECT_EQ(tags, "");
  EXPECT_EQ(message.getFlag(), 0);
  message.setFlag(2);
  EXPECT_EQ(message.getFlag(), 2);

  EXPECT_EQ(message.isWaitStoreMsgOK(), true);
  message.setWaitStoreMsgOK(false);
  EXPECT_EQ(message.isWaitStoreMsgOK(), false);
  message.setWaitStoreMsgOK(true);
  EXPECT_EQ(message.isWaitStoreMsgOK(), true);

  string keys(message.getTags());
  EXPECT_EQ(keys, "");
  message.setKeys("testKeys");
  EXPECT_EQ(message.getKeys(), "testKeys");

  EXPECT_EQ(message.getDelayTimeLevel(), 0);
  message.setDelayTimeLevel(1);
  EXPECT_EQ(message.getDelayTimeLevel(), 1);

  message.setSysFlag(1);
  EXPECT_EQ(message.getSysFlag(), 1);
}

TEST(message, properties) {
  MQMessage message;
  EXPECT_EQ(message.getProperties().size(), 1);
  EXPECT_STREQ(message.getProperty(MQMessage::PROPERTY_TRANSACTION_PREPARED).c_str(), "");

  message.setProperty(MQMessage::PROPERTY_TRANSACTION_PREPARED, "true");
  EXPECT_EQ(message.getProperties().size(), 2);
  EXPECT_EQ(message.getSysFlag(), 4);
  EXPECT_EQ(message.getProperty(MQMessage::PROPERTY_TRANSACTION_PREPARED), "true");

  message.setProperty(MQMessage::PROPERTY_TRANSACTION_PREPARED, "false");
  EXPECT_EQ(message.getProperties().size(), 2);
  EXPECT_EQ(message.getSysFlag(), 0);
  EXPECT_EQ(message.getProperty(MQMessage::PROPERTY_TRANSACTION_PREPARED), "false");

  map<string, string> newProperties;

  newProperties[MQMessage::PROPERTY_TRANSACTION_PREPARED] = "true";
  message.setProperties(newProperties);
  EXPECT_EQ(message.getSysFlag(), 4);
  EXPECT_EQ(message.getProperty(MQMessage::PROPERTY_TRANSACTION_PREPARED), "true");

  newProperties[MQMessage::PROPERTY_TRANSACTION_PREPARED] = "false";
  message.setProperties(newProperties);
  EXPECT_EQ(message.getSysFlag(), 0);
  EXPECT_EQ(message.getProperty(MQMessage::PROPERTY_TRANSACTION_PREPARED), "false");
}

TEST(message, Keys) {
  MQMessage message;
  vector<string> keys;
  keys.push_back("abc");
  keys.push_back("efg");
  keys.push_back("hij");
  message.setKeys(keys);
  EXPECT_EQ(message.getKeys(), "abc efg hij");
}
int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);

  // testing::GTEST_FLAG(filter) = "message.info";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
