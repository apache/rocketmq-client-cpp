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

#include "PermName.h"
#include "TopicConfig.h"
#include "TopicFilterType.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::PermName;
using rocketmq::TopicConfig;
using rocketmq::TopicFilterType;

TEST(TopicConfigTest, EncodeAndDecode) {
  TopicConfig topicConfig("testTopic", 4, 4, PermName::PERM_READ);
  std::string str = topicConfig.encode();
  EXPECT_EQ(str, "testTopic 4 4 4 0");

  TopicConfig topicDecodeConfig;
  topicDecodeConfig.decode(str);
  EXPECT_EQ(str, topicDecodeConfig.encode());
}

TEST(TopicConfigTest, GetterAndSetter) {
  TopicConfig topicConfig;

  topicConfig.set_topic_name("testTopic");
  EXPECT_EQ(topicConfig.topic_name(), "testTopic");

  topicConfig.set_read_queue_nums(4);
  EXPECT_EQ(topicConfig.read_queue_nums(), 4);

  topicConfig.set_write_queue_nums(4);
  EXPECT_EQ(topicConfig.write_queue_nums(), 4);

  topicConfig.set_perm(PermName::PERM_READ);
  EXPECT_EQ(topicConfig.perm(), PermName::PERM_READ);

  topicConfig.set_topic_filter_type(TopicFilterType::MULTI_TAG);
  EXPECT_EQ(topicConfig.topic_filter_type(), TopicFilterType::MULTI_TAG);
}

TEST(TopicConfigTest, Init) {
  TopicConfig topicConfig;
  EXPECT_TRUE(topicConfig.topic_name() == "");
  EXPECT_EQ(topicConfig.read_queue_nums(), TopicConfig::DEFAULT_READ_QUEUE_NUMS);
  EXPECT_EQ(topicConfig.write_queue_nums(), TopicConfig::DEFAULT_WRITE_QUEUE_NUMS);
  EXPECT_EQ(topicConfig.perm(), PermName::PERM_READ | PermName::PERM_WRITE);
  EXPECT_EQ(topicConfig.topic_filter_type(), TopicFilterType::SINGLE_TAG);

  TopicConfig twoTopicConfig("testTopic");
  EXPECT_EQ(twoTopicConfig.topic_name(), "testTopic");
  EXPECT_EQ(twoTopicConfig.read_queue_nums(), TopicConfig::DEFAULT_READ_QUEUE_NUMS);
  EXPECT_EQ(twoTopicConfig.write_queue_nums(), TopicConfig::DEFAULT_WRITE_QUEUE_NUMS);
  EXPECT_EQ(twoTopicConfig.perm(), PermName::PERM_READ | PermName::PERM_WRITE);
  EXPECT_EQ(twoTopicConfig.topic_filter_type(), TopicFilterType::SINGLE_TAG);

  TopicConfig threeTopicConfig("testTopic", 4, 4, PermName::PERM_READ);
  EXPECT_EQ(threeTopicConfig.topic_name(), "testTopic");
  EXPECT_EQ(threeTopicConfig.read_queue_nums(), 4);
  EXPECT_EQ(threeTopicConfig.write_queue_nums(), 4);
  EXPECT_EQ(threeTopicConfig.perm(), PermName::PERM_READ);
  EXPECT_EQ(threeTopicConfig.topic_filter_type(), TopicFilterType::SINGLE_TAG);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "TopicConfigTest.*";
  return RUN_ALL_TESTS();
}
