
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
#include <system_error>

#include "ClientManager.h"
#include "ClientManagerFactory.h"
#include "ClientManagerMock.h"
#include "SimpleConsumerImpl.h"
#include "StaticNameServerResolver.h"

ROCKETMQ_NAMESPACE_BEGIN

class SimpleConsumerTest : public testing::Test {
public:
  void SetUp() override {
    name_server_resolver_ = std::make_shared<StaticNameServerResolver>(name_server_list_);

    client_manager_ = std::make_shared<testing::NiceMock<ClientManagerMock>>();

    auto callback = [](const std::string& target_host, const Metadata& metadata, const QueryRouteRequest& request,
                       std::chrono::milliseconds timeout,
                       const std::function<void(const std::error_code&, const TopicRouteDataPtr& ptr)>& cb) {
      std::error_code ec;
      cb(ec, nullptr);
    };

    EXPECT_CALL(*client_manager_, resolveRoute).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(callback));

    ClientManagerFactory::getInstance().addClientManager(resource_namespace_, client_manager_);
  }

  void TearDown() override {
  }

protected:
  std::string resource_namespace_{"xds://"};
  std::string group_name_{"TestGroup"};
  std::string topic_{"TopicTest"};
  std::string name_server_list_{"10.0.0.1:9876"};
  std::shared_ptr<NameServerResolver> name_server_resolver_;
  std::shared_ptr<testing::NiceMock<ClientManagerMock>> client_manager_;
};

TEST_F(SimpleConsumerTest, testLifecycle) {
  auto consumer = std::make_shared<SimpleConsumerImpl>(group_name_);
  consumer->resourceNamespace(resource_namespace_);
  consumer->withNameServerResolver(name_server_resolver_);
  consumer->subscribe(topic_, "*", ExpressionType::TAG);

  consumer->start();
  consumer->shutdown();
}

ROCKETMQ_NAMESPACE_END