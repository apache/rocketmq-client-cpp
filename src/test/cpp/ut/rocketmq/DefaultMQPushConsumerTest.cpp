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
#include "rocketmq/DefaultMQPushConsumer.h"

#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class DefaultMQPushConsumerTest : public testing::Test {
public:
  DefaultMQPushConsumerTest() : group_name_("group-0"), consumer_(group_name_) {
  }

protected:
  std::string group_name_;
  DefaultMQPushConsumer consumer_;
};

TEST_F(DefaultMQPushConsumerTest, testGroupName) {
  EXPECT_EQ(group_name_, consumer_.groupName());
}

ROCKETMQ_NAMESPACE_END