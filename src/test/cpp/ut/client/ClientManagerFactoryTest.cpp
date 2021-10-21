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
#include "ClientManagerFactory.h"
#include "ClientConfigMock.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientManagerFactoryTest : public testing::Test {
public:
  void SetUp() override {
  }

  void TearDown() override {
  }

protected:
  testing::NiceMock<ClientConfigMock> client_config_;
  std::string resource_namespace_{"mq://test"};
};

TEST_F(ClientManagerFactoryTest, testGetClientManager) {
  EXPECT_CALL(client_config_, resourceNamespace)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::ReturnRef(resource_namespace_));
  ClientManagerPtr client_manager = ClientManagerFactory::getInstance().getClientManager(client_config_);
  EXPECT_TRUE(client_manager);
  client_manager->start();

  client_manager->shutdown();
}

ROCKETMQ_NAMESPACE_END