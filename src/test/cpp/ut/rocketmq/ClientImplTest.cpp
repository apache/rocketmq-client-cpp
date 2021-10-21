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
#include <chrono>
#include <memory>
#include <string>

#include "ClientImpl.h"
#include "ClientManagerFactory.h"
#include "ClientManagerMock.h"
#include "DynamicNameServerResolver.h"
#include "HttpClientMock.h"
#include "NameServerResolverMock.h"
#include "Scheduler.h"
#include "SchedulerImpl.h"
#include "TopAddressing.h"
#include "rocketmq/RocketMQ.h"

#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class TestClientImpl : public ClientImpl, public std::enable_shared_from_this<TestClientImpl> {
public:
  TestClientImpl(std::string group) : ClientImpl(std::move(group)) {
  }

  std::shared_ptr<ClientImpl> self() override {
    return shared_from_this();
  }

  void prepareHeartbeatData(HeartbeatRequest& request) override {
  }
};

class ClientImplTest : public testing::Test {
public:
  void SetUp() override {
    grpc_init();
    scheduler_ = std::make_shared<SchedulerImpl>();

    http_client_ = absl::make_unique<testing::NiceMock<HttpClientMock>>();
    name_server_resolver_ = std::make_shared<DynamicNameServerResolver>(endpoint_, std::chrono::seconds(1));
    client_manager_ = std::make_shared<testing::NiceMock<ClientManagerMock>>();
    ClientManagerFactory::getInstance().addClientManager(resource_namespace_, client_manager_);

    ON_CALL(*client_manager_, getScheduler).WillByDefault(testing::Return(scheduler_));
    ON_CALL(*client_manager_, start).WillByDefault([&]() { scheduler_->start(); });
    ON_CALL(*client_manager_, shutdown).WillByDefault([&]() { scheduler_->shutdown(); });

    client_ = std::make_shared<TestClientImpl>(group_);
    client_->withNameServerResolver(name_server_resolver_);
  }

  void TearDown() override {
    grpc_shutdown();
  }

protected:
  std::string endpoint_{"http://jmenv.tbsite.net:8080/rocketmq/nsaddr"};
  std::string resource_namespace_{"mq://test"};
  std::string group_{"Group-0"};
  std::shared_ptr<testing::NiceMock<ClientManagerMock>> client_manager_;
  SchedulerSharedPtr scheduler_;
  std::shared_ptr<TestClientImpl> client_;
  std::shared_ptr<DynamicNameServerResolver> name_server_resolver_;
  std::unique_ptr<testing::NiceMock<HttpClientMock>> http_client_;
};

TEST_F(ClientImplTest, testBasic) {
  std::string once{"10.0.0.1:9876"};
  std::string then{"10.0.0.1:9876;10.0.0.2:9876"};
  std::multimap<std::string, std::string> header;

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  int http_status = 200;
  auto once_cb =
      [&](HttpProtocol protocol, const std::string& host, std::uint16_t port, const std::string& path,
          const std::function<void(int, const std::multimap<std::string, std::string>&, const std::string&)>& cb) {
        cb(http_status, header, once);
      };
  auto then_cb =
      [&](HttpProtocol protocol, const std::string& host, std::uint16_t port, const std::string& path,
          const std::function<void(int, const std::multimap<std::string, std::string>&, const std::string&)>& cb) {
        cb(http_status, header, then);
        absl::MutexLock lk(&mtx);
        completed = true;
        cv.SignalAll();
      };

  EXPECT_CALL(*http_client_, get).WillOnce(testing::Invoke(once_cb)).WillRepeatedly(testing::Invoke(then_cb));
  name_server_resolver_->injectHttpClient(std::move(http_client_));

  client_->resourceNamespace(resource_namespace_);
  client_->start();
  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }

  ASSERT_TRUE(completed);

  // Now that the derivative class has closed its own resources, state of ClientImpl should be STOPPING.
  client_->state(State::STOPPING);
  client_->shutdown();
}

ROCKETMQ_NAMESPACE_END