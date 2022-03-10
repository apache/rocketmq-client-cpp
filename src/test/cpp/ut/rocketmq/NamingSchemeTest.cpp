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

#include "NamingScheme.h"
#include "rocketmq/RocketMQ.h"

#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class NamingSchemeTest : public testing::Test {
public:
  void SetUp() override {
  }

  void TearDown() override {
  }

protected:
  NamingScheme naming_scheme_;
};

TEST_F(NamingSchemeTest, testBuildAddress) {
  std::string address = "www.baidu.com:80";
  std::string result = naming_scheme_.buildAddress({address});
  ASSERT_EQ("dns:www.baidu.com:80", result);

  address = "8.8.8.8:1234";
  result = naming_scheme_.buildAddress({address});
  ASSERT_EQ("ipv4:8.8.8.8:1234", result);

  result = naming_scheme_.buildAddress({address, "4.4.4.4:1234"});
  ASSERT_EQ("ipv4:8.8.8.8:1234,4.4.4.4:1234", result);
}

ROCKETMQ_NAMESPACE_END