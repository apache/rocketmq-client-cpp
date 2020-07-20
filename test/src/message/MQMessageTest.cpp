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

#include <map>
#include <string>

#include "MQMessage.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQMessage;
using rocketmq::MQMessageConst;

TEST(MessageTest, Init) {
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
}

TEST(MessageTest, GetterAndSetter) {
  MQMessage message;

  EXPECT_EQ(message.getTopic(), "");  // default
  message.setTopic("testTopic");
  EXPECT_EQ(message.getTopic(), "testTopic");

  const char* topic = "testTopic";
  message.setTopic(topic, 5);
  EXPECT_EQ(message.getTopic(), "testT");

  EXPECT_EQ(message.getBody(), "");  // default
  message.setBody("testBody");
  EXPECT_EQ(message.getBody(), "testBody");

  EXPECT_EQ(message.getTags(), "");  // default
  message.setTags("testTags");
  EXPECT_EQ(message.getTags(), "testTags");

  EXPECT_EQ(message.getKeys(), "");  // default
  message.setKeys("testKeys");
  EXPECT_EQ(message.getKeys(), "testKeys");

  EXPECT_EQ(message.getFlag(), 0);  // default
  message.setFlag(2);
  EXPECT_EQ(message.getFlag(), 2);

  EXPECT_EQ(message.isWaitStoreMsgOK(), true);  // default
  message.setWaitStoreMsgOK(false);
  EXPECT_EQ(message.isWaitStoreMsgOK(), false);
  message.setWaitStoreMsgOK(true);
  EXPECT_EQ(message.isWaitStoreMsgOK(), true);

  EXPECT_EQ(message.getDelayTimeLevel(), 0);  // default
  message.setDelayTimeLevel(1);
  EXPECT_EQ(message.getDelayTimeLevel(), 1);
}

TEST(MessageTest, Properties) {
  MQMessage message;

  EXPECT_EQ(message.getProperties().size(), 1);
  EXPECT_EQ(message.getProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED), "");

  message.putProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED, "true");
  EXPECT_EQ(message.getProperties().size(), 2);
  EXPECT_EQ(message.getProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED), "true");

  std::map<std::string, std::string> newProperties;
  newProperties[MQMessageConst::PROPERTY_TRANSACTION_PREPARED] = "false";
  message.setProperties(newProperties);
  EXPECT_EQ(message.getProperties().size(), 1);
  EXPECT_EQ(message.getProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED), "false");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "MessageTest.*";
  return RUN_ALL_TESTS();
}
