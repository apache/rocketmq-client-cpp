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
#include <iostream>
#include <map>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "MQClientManager.h"

using namespace std;
using namespace rocketmq;
using rocketmq::MQClientFactory;
using rocketmq::MQClientManager;
using ::testing::InitGoogleMock;
using ::testing::InitGoogleTest;
using testing::Return;

TEST(MQClientManagerTest, getClientFactory) {
  string clientId = "testClientId";
  string unitName = "central";
  MQClientFactory* factory = MQClientManager::getInstance()->getMQClientFactory(clientId, 1, 1000, 3000, unitName, true,
                                                                                DEFAULT_SSL_PROPERTY_FILE);
  MQClientFactory* factory2 = MQClientManager::getInstance()->getMQClientFactory(clientId, 1, 1000, 3000, unitName,
                                                                                 true, DEFAULT_SSL_PROPERTY_FILE);
  EXPECT_EQ(factory, factory2);
  factory->shutdown();

  MQClientManager::getInstance()->removeClientFactory(clientId);
}
TEST(MQClientManagerTest, removeClientFactory) {
  string clientId = "testClientId";
  MQClientManager::getInstance()->removeClientFactory(clientId);
}
int main(int argc, char* argv[]) {
  InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
