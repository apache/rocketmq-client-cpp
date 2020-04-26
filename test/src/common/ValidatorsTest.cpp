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

#include <string>

#include "MQClientException.h"
#include "MQMessage.h"
#include "Validators.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQClientException;
using rocketmq::MQMessage;
using rocketmq::Validators;

TEST(ValidatorsTest, RegularExpressionMatcher) {
  EXPECT_FALSE(Validators::regularExpressionMatcher("", ""));
  EXPECT_TRUE(Validators::regularExpressionMatcher("123456", ""));
  EXPECT_FALSE(Validators::regularExpressionMatcher("123%456", Validators::validPatternStr));
  EXPECT_TRUE(Validators::regularExpressionMatcher("123456", Validators::validPatternStr));
}

TEST(ValidatorsTest, GetGroupWithRegularExpression) {
  EXPECT_EQ(Validators::getGroupWithRegularExpression("", ""), "");
}

TEST(ValidatorsTest, CheckTopic) {
  EXPECT_THROW(Validators::checkTopic(""), MQClientException);
  std::string exceptionTopic = "1234567890";
  for (int i = 0; i < 25; i++) {
    exceptionTopic.append("1234567890");
  }
  EXPECT_THROW(Validators::checkTopic(exceptionTopic), MQClientException);
  EXPECT_THROW(Validators::checkTopic("TBW102"), MQClientException);
}

TEST(ValidatorsTest, CheckGroup) {
  EXPECT_THROW(Validators::checkGroup(""), MQClientException);
  std::string exceptionTopic = "1234567890";
  for (int i = 0; i < 25; i++) {
    exceptionTopic.append("1234567890");
  }
  EXPECT_THROW(Validators::checkGroup(exceptionTopic), MQClientException);
}

TEST(ValidatorsTest, CheckMessage) {
  MQMessage message("testTopic", "");
  EXPECT_THROW(Validators::checkMessage(MQMessage("testTopic", ""), 1), MQClientException);
  EXPECT_THROW(Validators::checkMessage(MQMessage("testTopic", "123"), 2), MQClientException);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "ValidatorsTest.Skipped";
  return RUN_ALL_TESTS();
}
