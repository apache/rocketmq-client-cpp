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

#include "VirtualEnvUtil.h"

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::VirtualEnvUtil;

TEST(VirtualEnvUtilTest, BuildWithProjectGroup) {
  EXPECT_EQ(VirtualEnvUtil::buildWithProjectGroup("origin", ""), "origin");
  EXPECT_EQ(VirtualEnvUtil::buildWithProjectGroup("origin", "123"), "origin%PROJECT_123%");
}

TEST(VirtualEnvUtilTest, ClearProjectGroup) {}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "VirtualEnvUtilTest.*";
  return RUN_ALL_TESTS();
}
