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

#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "NamesrvConfig.h"

using std::string;

using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::NamesrvConfig;

TEST(namesrvConfig, init) {
  NamesrvConfig namesrvConfig;

  const string home = "/home/rocketmq";
  namesrvConfig.setRocketmqHome(home);
  EXPECT_EQ(namesrvConfig.getRocketmqHome(), "/home/rocketmq");

  namesrvConfig.setKvConfigPath("/home/rocketmq");
  EXPECT_EQ(namesrvConfig.getKvConfigPath(), "/home/rocketmq");
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "namesrvConfig.init";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
