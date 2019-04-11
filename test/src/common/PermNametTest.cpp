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
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "PermName.h"

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::PermName;

TEST(permName, perm2String) {
  EXPECT_EQ(PermName::perm2String(0), "---");
  EXPECT_EQ(PermName::perm2String(1), "--X");
  EXPECT_EQ(PermName::perm2String(2), "-W");
  EXPECT_EQ(PermName::perm2String(3), "-WX");
  EXPECT_EQ(PermName::perm2String(4), "R--");
  EXPECT_EQ(PermName::perm2String(5), "R-X");
  EXPECT_EQ(PermName::perm2String(6), "RW");
  EXPECT_EQ(PermName::perm2String(7), "RWX");
  EXPECT_EQ(PermName::perm2String(8), "---");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "permName.perm2String";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
