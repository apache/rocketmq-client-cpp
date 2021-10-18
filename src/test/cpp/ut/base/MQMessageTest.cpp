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
#include "rocketmq/MQMessage.h"
#include "gtest/gtest.h"
#include <cstring>

ROCKETMQ_NAMESPACE_BEGIN

class MQMessageTest : public testing::Test {
public:
  void SetUp() override {
    message.setTopic(topic_);
    message.setKey(key_);
    message.setBody(body_data_, strlen(body_data_));
    message.setDelayTimeLevel(delay_level_);
  }

  void TearDown() override {
  }

protected:
  std::string topic_{"Test"};
  std::string key_{"key0"};
  const char* body_data_{"body content"};
  int delay_level_{1};
  MQMessage message;
};

TEST_F(MQMessageTest, testAssignment) {
  MQMessage msg;
  msg = message;
  EXPECT_EQ(msg.getTopic(), topic_);
  EXPECT_EQ(msg.getDelayTimeLevel(), delay_level_);
  EXPECT_EQ(*msg.getKeys().begin(), key_);
  EXPECT_EQ(msg.getBody(), body_data_);
}

TEST_F(MQMessageTest, testProperty) {
  std::string key{"k"};
  std::string value{"value"};
  message.setProperty(key, value);
  auto prop_value = message.getProperty(key);
  EXPECT_EQ(value, prop_value);
}

ROCKETMQ_NAMESPACE_END