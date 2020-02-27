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

#include "VirtualEnvUtil.h"

using std::string;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::VirtualEnvUtil;

VirtualEnvUtil virtualEnvUtil;

TEST(virtualEnvUtil, buildWithProjectGroup) {
  string origin = "origin";
  string projectGroup;
  EXPECT_EQ(virtualEnvUtil.buildWithProjectGroup(origin, string()), origin);

  EXPECT_EQ(virtualEnvUtil.buildWithProjectGroup(origin, string("123")), origin);
}

TEST(virtualEnvUtil, clearProjectGroup) {}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);

  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "messageExt.init";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
