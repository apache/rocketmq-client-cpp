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

#include "MQClientException.h"
#include "MQMessage.h"
#include "Validators.h"

using std::string;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQClientException;
using rocketmq::MQMessage;
using rocketmq::Validators;

TEST(validators, regularExpressionMatcher) {
  EXPECT_FALSE(Validators::regularExpressionMatcher(string(), string()));

  EXPECT_TRUE(Validators::regularExpressionMatcher(string("123456"), string()));

  EXPECT_TRUE(Validators::regularExpressionMatcher(string("123456"), string("123")));
}

TEST(validators, getGroupWithRegularExpression) {
  EXPECT_EQ(Validators::getGroupWithRegularExpression(string(), string()), "");
}

TEST(validators, checkTopic) {
  EXPECT_THROW(Validators::checkTopic(string()), MQClientException);
  string exceptionTopic = "1234567890";
  for (int i = 0; i < 25; i++) {
    exceptionTopic.append("1234567890");
  }
  EXPECT_THROW(Validators::checkTopic(exceptionTopic), MQClientException);

  EXPECT_THROW(Validators::checkTopic("TBW102"), MQClientException);
}

TEST(validators, checkGroup) {
  EXPECT_THROW(Validators::checkGroup(string()), MQClientException);
  string exceptionTopic = "1234567890";
  for (int i = 0; i < 25; i++) {
    exceptionTopic.append("1234567890");
  }
  EXPECT_THROW(Validators::checkGroup(exceptionTopic), MQClientException);
}

TEST(validators, checkMessage) {
  MQMessage message("testTopic", string());

  EXPECT_THROW(Validators::checkMessage(MQMessage("testTopic", string()), 1), MQClientException);

  EXPECT_THROW(Validators::checkMessage(MQMessage("testTopic", string("123")), 2), MQClientException);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "validators.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
