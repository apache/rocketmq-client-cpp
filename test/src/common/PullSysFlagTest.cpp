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

#include "PullSysFlag.h"

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::PullSysFlag;

TEST(pullSysFlag, flag) {
  EXPECT_EQ(PullSysFlag::buildSysFlag(false, false, false, false), 0);

  EXPECT_EQ(PullSysFlag::buildSysFlag(true, false, false, false), 1);
  EXPECT_EQ(PullSysFlag::buildSysFlag(true, true, false, false), 3);
  EXPECT_EQ(PullSysFlag::buildSysFlag(true, true, true, false), 7);
  EXPECT_EQ(PullSysFlag::buildSysFlag(true, true, true, true), 15);

  EXPECT_EQ(PullSysFlag::buildSysFlag(false, true, false, false), 2);
  EXPECT_EQ(PullSysFlag::buildSysFlag(false, true, true, false), 6);
  EXPECT_EQ(PullSysFlag::buildSysFlag(false, true, true, true), 14);

  EXPECT_EQ(PullSysFlag::buildSysFlag(false, false, true, false), 4);
  EXPECT_EQ(PullSysFlag::buildSysFlag(false, false, true, true), 12);

  EXPECT_EQ(PullSysFlag::buildSysFlag(false, false, false, true), 8);

  int FLAG_COMMIT_OFFSET = 0x1 << 0;
  int FLAG_SUSPEND = 0x1 << 1;
  int FLAG_SUBSCRIPTION = 0x1 << 2;
  int FLAG_CLASS_FILTER = 0x1 << 3;

  for (int i = 0; i < 16; i++) {
    if ((i & FLAG_COMMIT_OFFSET) == FLAG_COMMIT_OFFSET) {
      EXPECT_TRUE(PullSysFlag::hasCommitOffsetFlag(i));
    } else {
      EXPECT_FALSE(PullSysFlag::hasCommitOffsetFlag(i));
    }

    if ((i & FLAG_SUSPEND) == FLAG_SUSPEND) {
      EXPECT_TRUE(PullSysFlag::hasSuspendFlag(i));
    } else {
      EXPECT_FALSE(PullSysFlag::hasSuspendFlag(i));
    }

    if ((i & FLAG_SUBSCRIPTION) == FLAG_SUBSCRIPTION) {
      EXPECT_TRUE(PullSysFlag::hasSubscriptionFlag(i));
    } else {
      EXPECT_FALSE(PullSysFlag::hasSubscriptionFlag(i));
    }

    if ((i & FLAG_CLASS_FILTER) == FLAG_CLASS_FILTER) {
      EXPECT_TRUE(PullSysFlag::hasClassFilterFlag(i));
    } else {
      EXPECT_FALSE(PullSysFlag::hasClassFilterFlag(i));
    }

    if ((i & FLAG_COMMIT_OFFSET) == FLAG_COMMIT_OFFSET) {
      EXPECT_TRUE(PullSysFlag::hasCommitOffsetFlag(i));
    } else {
      EXPECT_FALSE(PullSysFlag::hasCommitOffsetFlag(i));
    }

    if (i == 0 || i == 1) {
      EXPECT_EQ(PullSysFlag::clearCommitOffsetFlag(i), 0);
    } else {
      EXPECT_TRUE(PullSysFlag::clearCommitOffsetFlag(i) > 0);
    }
  }
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "pullSysFlag.flag";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
