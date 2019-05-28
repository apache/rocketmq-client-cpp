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
#include "string.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "PermName.h"
#include "TopicConfig.h"
#include "TopicFilterType.h"

using std::string;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::PermName;
using rocketmq::TopicConfig;
using rocketmq::TopicFilterType;

TEST(topicConfig, encodeAndDecode) {
  TopicConfig topicConfig("testTopic", 4, 4, PermName::PERM_READ);
  string str = topicConfig.encode();

  TopicConfig topicDecodeConfig;
  topicDecodeConfig.decode(str);
  EXPECT_EQ(str, topicDecodeConfig.encode());
}

TEST(topicConfig, info) {
  TopicConfig topicConfig;

  topicConfig.setTopicName("testTopic");
  EXPECT_EQ(topicConfig.getTopicName(), "testTopic");

  topicConfig.setReadQueueNums(4);
  EXPECT_EQ(topicConfig.getReadQueueNums(), 4);

  topicConfig.setWriteQueueNums(4);
  EXPECT_EQ(topicConfig.getWriteQueueNums(), 4);

  topicConfig.setPerm(PermName::PERM_READ);
  EXPECT_EQ(topicConfig.getPerm(), PermName::PERM_READ);

  topicConfig.setTopicFilterType(TopicFilterType::MULTI_TAG);
  EXPECT_EQ(topicConfig.getTopicFilterType(), TopicFilterType::MULTI_TAG);
}

TEST(topicConfig, init) {
  TopicConfig topicConfig;
  EXPECT_TRUE(topicConfig.getTopicName() == "");
  EXPECT_EQ(topicConfig.getReadQueueNums(), TopicConfig::DefaultReadQueueNums);
  EXPECT_EQ(topicConfig.getWriteQueueNums(), TopicConfig::DefaultWriteQueueNums);
  EXPECT_EQ(topicConfig.getPerm(), PermName::PERM_READ | PermName::PERM_WRITE);
  EXPECT_EQ(topicConfig.getTopicFilterType(), TopicFilterType::SINGLE_TAG);

  TopicConfig twoTopicConfig("testTopic");
  EXPECT_EQ(twoTopicConfig.getTopicName(), "testTopic");
  EXPECT_EQ(twoTopicConfig.getReadQueueNums(), TopicConfig::DefaultReadQueueNums);
  EXPECT_EQ(twoTopicConfig.getWriteQueueNums(), TopicConfig::DefaultWriteQueueNums);
  EXPECT_EQ(twoTopicConfig.getPerm(), PermName::PERM_READ | PermName::PERM_WRITE);
  EXPECT_EQ(twoTopicConfig.getTopicFilterType(), TopicFilterType::SINGLE_TAG);

  TopicConfig threeTopicConfig("testTopic", 4, 4, PermName::PERM_READ);
  EXPECT_EQ(threeTopicConfig.getTopicName(), "testTopic");
  EXPECT_EQ(threeTopicConfig.getReadQueueNums(), 4);
  EXPECT_EQ(threeTopicConfig.getWriteQueueNums(), 4);
  EXPECT_EQ(threeTopicConfig.getPerm(), PermName::PERM_READ);
  EXPECT_EQ(threeTopicConfig.getTopicFilterType(), TopicFilterType::SINGLE_TAG);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "topicConfig.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
