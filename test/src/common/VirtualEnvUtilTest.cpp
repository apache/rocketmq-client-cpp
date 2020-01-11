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

TEST(virtualEnvUtil, buildWithProjectGroup) {
  string origin = "origin";
  string originWithGroupA = "origin%PROJECT_testGroupA%";
  string originWithGroupB = "origin%PROJECT_testGroupB%";
  string originWithGroupAB = "origin%PROJECT_testGroupA%%PROJECT_testGroupB%";
  string projectGroupA = "testGroupA";
  string projectGroupB = "testGroupB";
  EXPECT_EQ(VirtualEnvUtil::buildWithProjectGroup(origin, string()), origin);
  EXPECT_EQ(VirtualEnvUtil::buildWithProjectGroup(origin, projectGroupA), originWithGroupA);
  EXPECT_EQ(VirtualEnvUtil::buildWithProjectGroup(originWithGroupA, projectGroupA), originWithGroupA);
  EXPECT_EQ(VirtualEnvUtil::buildWithProjectGroup(originWithGroupA, projectGroupB), originWithGroupAB);
}

TEST(virtualEnvUtil, clearProjectGroup) {
  string origin = "origin";
  string originWithGroup = "origin%PROJECT_testGroup%";
  string projectGroup = "testGroup";
  string projectGroupB = "testGroupB";
  EXPECT_EQ(VirtualEnvUtil::clearProjectGroup(origin, string()), origin);
  EXPECT_EQ(VirtualEnvUtil::clearProjectGroup(originWithGroup, string()), originWithGroup);
  EXPECT_EQ(VirtualEnvUtil::clearProjectGroup(originWithGroup, projectGroupB), originWithGroup);
  EXPECT_EQ(VirtualEnvUtil::clearProjectGroup(origin, projectGroup), origin);
  EXPECT_EQ(VirtualEnvUtil::clearProjectGroup(originWithGroup, projectGroup), origin);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);

  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "virtualEnvUtil.*";
  int iTest = RUN_ALL_TESTS();
  return iTest;
}
