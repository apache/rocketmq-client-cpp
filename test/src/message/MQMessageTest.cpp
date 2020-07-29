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
  EXPECT_EQ(messageOne.topic(), "");
  EXPECT_EQ(messageOne.body(), "");
  EXPECT_EQ(messageOne.tags(), "");
  EXPECT_EQ(messageOne.flag(), 0);

  MQMessage messageTwo("test", "testBody");
  EXPECT_EQ(messageTwo.topic(), "test");
  EXPECT_EQ(messageTwo.body(), "testBody");
  EXPECT_EQ(messageTwo.tags(), "");
  EXPECT_EQ(messageTwo.flag(), 0);

  MQMessage messageThree("test", "tagTest", "testBody");
  EXPECT_EQ(messageThree.topic(), "test");
  EXPECT_EQ(messageThree.body(), "testBody");
  EXPECT_EQ(messageThree.tags(), "tagTest");
  EXPECT_EQ(messageThree.flag(), 0);

  MQMessage messageFour("test", "tagTest", "testKey", "testBody");
  EXPECT_EQ(messageFour.topic(), "test");
  EXPECT_EQ(messageFour.body(), "testBody");
  EXPECT_EQ(messageFour.tags(), "tagTest");
  EXPECT_EQ(messageFour.keys(), "testKey");
  EXPECT_EQ(messageFour.flag(), 0);

  MQMessage messageFive("test", "tagTest", "testKey", 1, "testBody", 2);
  EXPECT_EQ(messageFive.topic(), "test");
  EXPECT_EQ(messageFive.body(), "testBody");
  EXPECT_EQ(messageFive.tags(), "tagTest");
  EXPECT_EQ(messageFive.keys(), "testKey");
  EXPECT_EQ(messageFive.flag(), 1);

  MQMessage messageSix(messageFive);
  EXPECT_EQ(messageSix.topic(), "test");
  EXPECT_EQ(messageSix.body(), "testBody");
  EXPECT_EQ(messageSix.tags(), "tagTest");
  EXPECT_EQ(messageSix.keys(), "testKey");
  EXPECT_EQ(messageSix.flag(), 1);
}

TEST(MessageTest, GetterAndSetter) {
  MQMessage message;

  EXPECT_EQ(message.topic(), "");  // default
  message.set_topic("testTopic");
  EXPECT_EQ(message.topic(), "testTopic");

  const char* topic = "testTopic";
  message.set_topic(topic, 5);
  EXPECT_EQ(message.topic(), "testT");

  EXPECT_EQ(message.body(), "");  // default
  message.set_body("testBody");
  EXPECT_EQ(message.body(), "testBody");

  EXPECT_EQ(message.tags(), "");  // default
  message.set_tags("testTags");
  EXPECT_EQ(message.tags(), "testTags");

  EXPECT_EQ(message.keys(), "");  // default
  message.set_keys("testKeys");
  EXPECT_EQ(message.keys(), "testKeys");

  EXPECT_EQ(message.flag(), 0);  // default
  message.set_flag(2);
  EXPECT_EQ(message.flag(), 2);

  EXPECT_EQ(message.wait_store_msg_ok(), true);  // default
  message.set_wait_store_msg_ok(false);
  EXPECT_EQ(message.wait_store_msg_ok(), false);
  message.set_wait_store_msg_ok(true);
  EXPECT_EQ(message.wait_store_msg_ok(), true);

  EXPECT_EQ(message.delay_time_level(), 0);  // default
  message.set_delay_time_level(1);
  EXPECT_EQ(message.delay_time_level(), 1);
}

TEST(MessageTest, Properties) {
  MQMessage message;

  EXPECT_EQ(message.properties().size(), 1);
  EXPECT_EQ(message.getProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED), "");

  message.putProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED, "true");
  EXPECT_EQ(message.properties().size(), 2);
  EXPECT_EQ(message.getProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED), "true");

  std::map<std::string, std::string> newProperties;
  newProperties[MQMessageConst::PROPERTY_TRANSACTION_PREPARED] = "false";
  message.set_properties(newProperties);
  EXPECT_EQ(message.properties().size(), 1);
  EXPECT_EQ(message.getProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED), "false");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "MessageTest.*";
  return RUN_ALL_TESTS();
}
