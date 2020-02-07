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
#include <stdio.h>

#include "MQMessageId.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace std;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

using rocketmq::MQMessageId;

TEST(messageId, id) {
  int host;
  int port;
  sockaddr addr = rocketmq::IPPort2socketAddress(inet_addr("127.0.0.1"), 10091);
  MQMessageId id(addr, 1024);

  rocketmq::socketAddress2IPPort(id.getAddress(), host, port);
  EXPECT_EQ(host, inet_addr("127.0.0.1"));
  EXPECT_EQ(port, 10091);
  EXPECT_EQ(id.getOffset(), 1024);

  id.setAddress(rocketmq::IPPort2socketAddress(inet_addr("127.0.0.2"), 10092));
  id.setOffset(2048);

  rocketmq::socketAddress2IPPort(id.getAddress(), host, port);
  EXPECT_EQ(host, inet_addr("127.0.0.2"));
  EXPECT_EQ(port, 10092);
  EXPECT_EQ(id.getOffset(), 2048);

  MQMessageId id2 = id;
  EXPECT_EQ(id2.getOffset(), 2048);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);

  testing::GTEST_FLAG(filter) = "messageId.*";
  int itestts = RUN_ALL_TESTS();
  return itestts;
}
