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
#include "SocketUtil.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::InitGoogleMock;
using testing::InitGoogleTest;
using testing::Return;

using rocketmq::ByteArray;

using namespace rocketmq;

TEST(SocketUtilTest, Convert) {
  char ip[] = {0x7F, 0x00, 0x00, 0x01};
  struct sockaddr* sa = IPPortToSockaddr(ByteArray(ip, sizeof(ip)), 0x276B);
  struct sockaddr_in* sin = (struct sockaddr_in*)sa;
  EXPECT_EQ(sin->sin_addr.s_addr, 0x0100007F);
  EXPECT_EQ(sin->sin_port, 0x6B27);

  EXPECT_EQ(SockaddrToString(sa), "127.0.0.1:10091");

  sa = StringToSockaddr("127.0.0.1:10091");
  sin = (struct sockaddr_in*)sa;
  EXPECT_EQ(sin->sin_addr.s_addr, 0x0100007F);
  EXPECT_EQ(sin->sin_port, 0x6B27);
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  testing::GTEST_FLAG(throw_on_failure) = true;
  testing::GTEST_FLAG(filter) = "SocketUtilTest.*";
  return RUN_ALL_TESTS();
}
