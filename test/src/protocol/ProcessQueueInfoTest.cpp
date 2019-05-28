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

#include "MQMessageExt.h"
#include "ProcessQueueInfo.h"

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQMessageExt;
using rocketmq::ProcessQueueInfo;

TEST(processQueueInfo, init) {
  ProcessQueueInfo processQueueInfo;
  EXPECT_EQ(processQueueInfo.commitOffset, 0);
  EXPECT_EQ(processQueueInfo.cachedMsgMinOffset, 0);
  EXPECT_EQ(processQueueInfo.cachedMsgMaxOffset, 0);
  EXPECT_EQ(processQueueInfo.cachedMsgCount, 0);
  EXPECT_EQ(processQueueInfo.transactionMsgMinOffset, 0);
  EXPECT_EQ(processQueueInfo.transactionMsgMaxOffset, 0);
  EXPECT_EQ(processQueueInfo.transactionMsgCount, 0);
  EXPECT_EQ(processQueueInfo.locked, false);
  EXPECT_EQ(processQueueInfo.tryUnlockTimes, 0);
  EXPECT_EQ(processQueueInfo.lastLockTimestamp, 123);
  EXPECT_EQ(processQueueInfo.droped, false);
  EXPECT_EQ(processQueueInfo.lastPullTimestamp, 0);
  EXPECT_EQ(processQueueInfo.lastConsumeTimestamp, 0);

  processQueueInfo.setLocked(true);
  EXPECT_EQ(processQueueInfo.isLocked(), true);

  processQueueInfo.setDroped(true);
  EXPECT_EQ(processQueueInfo.isDroped(), true);

  processQueueInfo.setCommitOffset(456);
  EXPECT_EQ(processQueueInfo.getCommitOffset(), 456);

  Json::Value outJson = processQueueInfo.toJson();

  EXPECT_EQ(outJson["commitOffset"], "456");
  EXPECT_EQ(outJson["cachedMsgMinOffset"], "0");
  EXPECT_EQ(outJson["cachedMsgMaxOffset"], "0");
  EXPECT_EQ(outJson["cachedMsgCount"].asInt(), 0);
  EXPECT_EQ(outJson["transactionMsgMinOffset"], "0");
  EXPECT_EQ(outJson["transactionMsgMaxOffset"], "0");
  EXPECT_EQ(outJson["transactionMsgCount"].asInt(), 0);
  EXPECT_EQ(outJson["locked"].asBool(), true);
  EXPECT_EQ(outJson["tryUnlockTimes"].asInt(), 0);
  EXPECT_EQ(outJson["lastLockTimestamp"], "123");
  EXPECT_EQ(outJson["droped"].asBool(), true);
  EXPECT_EQ(outJson["lastPullTimestamp"], "0");
  EXPECT_EQ(outJson["lastConsumeTimestamp"], "0");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "processQueueInfo.init";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
