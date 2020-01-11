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

#include "MQVersion.h"

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQVersion;
using rocketmq::RocketMQCPPClientVersion;

TEST(MQVersionTest, Version2String) {
  for (int v = MQVersion::V3_0_0_SNAPSHOT; v <= MQVersion::HIGHER_VERSION; v++) {
    EXPECT_STREQ(MQVersion::GetVersionDesc(v), RocketMQCPPClientVersion[v]);
  }
  EXPECT_STREQ(MQVersion::GetVersionDesc(-100), MQVersion::GetVersionDesc(MQVersion::V3_0_0_SNAPSHOT));
  EXPECT_STREQ(MQVersion::GetVersionDesc(MQVersion::V4_6_0), "V4_6_0");
  EXPECT_STREQ(MQVersion::GetVersionDesc(MQVersion::HIGHER_VERSION + 100),
               MQVersion::GetVersionDesc(MQVersion::HIGHER_VERSION));
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "MQVersionTest.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
