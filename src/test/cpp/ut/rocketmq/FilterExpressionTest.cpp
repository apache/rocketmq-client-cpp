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
#include "FilterExpression.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class FilterExpressionTest : public testing::Test {
public:
  FilterExpressionTest() : filter_expression_("TagA") {
  }

  void SetUp() override {
  }

  void TearDown() override {
  }

protected:
  FilterExpression filter_expression_;
};

TEST_F(FilterExpressionTest, testAccept) {
  MQMessageExt message;
  message.setTags("TagA");
  EXPECT_TRUE(filter_expression_.accept(message));

  MQMessageExt message2;
  message2.setTags("TagB");
  EXPECT_FALSE(filter_expression_.accept(message2));
}

ROCKETMQ_NAMESPACE_END