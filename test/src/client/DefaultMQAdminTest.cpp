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

#include <map>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "DefaultMQAdmin.h"

using namespace std;
using namespace rocketmq;
using rocketmq::DefaultMQAdmin;
using testing::_;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

TEST(DefaultMQAdminTest, init) {
  DefaultMQAdmin* impl = new DefaultMQAdmin("testMQAdminGroup");
  EXPECT_EQ(impl->getGroupName(), "testMQAdminGroup");
  impl->setUnitName("testUnit");
  EXPECT_EQ(impl->getUnitName(), "testUnit");
  impl->setTcpTransportPullThreadNum(64);
  EXPECT_EQ(impl->getTcpTransportPullThreadNum(), 64);
  impl->setTcpTransportConnectTimeout(2000);
  EXPECT_EQ(impl->getTcpTransportConnectTimeout(), 2000);
  impl->setTcpTransportTryLockTimeout(3000);
  EXPECT_EQ(impl->getTcpTransportTryLockTimeout(), 3);
  impl->setNamesrvAddr("http://rocketmq.nameserver.com");
  EXPECT_EQ(impl->getNamesrvAddr(), "rocketmq.nameserver.com");
  impl->setNameSpace("MQ_INST_NAMESPACE_TEST");
  EXPECT_EQ(impl->getNameSpace(), "MQ_INST_NAMESPACE_TEST");
  impl->setMessageTrace(true);
  EXPECT_TRUE(impl->getMessageTrace());
}

int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
