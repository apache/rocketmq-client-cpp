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
#include <memory>
#include <system_error>

#include "ClientManagerFactory.h"
#include "ClientManagerMock.h"
#include "ProducerImpl.h"
#include "Scheduler.h"
#include "SchedulerImpl.h"
#include "StaticNameServerResolver.h"
#include "TopicRouteData.h"
#include "rocketmq/AsyncCallback.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQSelector.h"
#include "rocketmq/RocketMQ.h"
#include "rocketmq/SendResult.h"

ROCKETMQ_NAMESPACE_BEGIN

class ProducerImplTest : public testing::Test {
public:
  ProducerImplTest() : credentials_provider_(std::make_shared<StaticCredentialsProvider>(access_key_, access_secret_)) {
  }

  void SetUp() override {
    grpc_init();
    scheduler_ = std::make_shared<SchedulerImpl>();
    scheduler_->start();
    name_server_resolver_ = std::make_shared<StaticNameServerResolver>(name_server_list_);
    client_manager_ = std::make_shared<testing::NiceMock<ClientManagerMock>>();
    ON_CALL(*client_manager_, getScheduler).WillByDefault(testing::Return(scheduler_));
    ClientManagerFactory::getInstance().addClientManager(resource_namespace_, client_manager_);
    producer_ = std::make_shared<ProducerImpl>(group_);
    producer_->resourceNamespace(resource_namespace_);
    producer_->withNameServerResolver(name_server_resolver_);
    producer_->setCredentialsProvider(credentials_provider_);

    {
      std::vector<Partition> partitions;
      Topic topic(resource_namespace_, topic_);
      std::vector<Address> broker_addresses{Address(broker_host_, broker_port_)};
      ServiceAddress service_address(AddressScheme::IPv4, broker_addresses);
      Broker broker(broker_name_, broker_id_, service_address);
      Partition partition(topic, queue_id_, Permission::READ_WRITE, broker);
      partitions.emplace_back(partition);
      std::string debug_string;
      topic_route_data_ = std::make_shared<TopicRouteData>(partitions, debug_string);
    }
  }

  void TearDown() override {
    scheduler_->shutdown();
    grpc_shutdown();
  }

protected:
  SchedulerSharedPtr scheduler_;
  std::shared_ptr<testing::NiceMock<ClientManagerMock>> client_manager_;
  std::shared_ptr<ProducerImpl> producer_;
  std::string name_server_list_{"10.0.0.1:9876"};
  std::shared_ptr<NameServerResolver> name_server_resolver_;
  std::string resource_namespace_{"mq://test"};
  std::string group_{"CID_test"};
  std::string topic_{"Topic0"};
  int queue_id_{1};
  std::string tag_{"TagA"};
  std::string key_{"key-0"};
  std::string message_group_{"group-0"};
  std::string broker_name_{"broker-a"};
  int broker_id_{0};
  std::string message_body_{"Message Body Content"};
  std::string broker_host_{"10.0.0.1"};
  int broker_port_{10911};
  TopicRouteDataPtr topic_route_data_;
  std::string access_key_{"access_key"};
  std::string access_secret_{"access_secret"};
  std::shared_ptr<CredentialsProvider> credentials_provider_;
};

TEST_F(ProducerImplTest, testStartShutdown) {
  producer_->start();
  producer_->shutdown();
}

TEST_F(ProducerImplTest, testSend) {
  auto mock_resolve_route =
      [this](const std::string& target_host, const Metadata& metadata, const QueryRouteRequest& request,
             std::chrono::milliseconds timeout,
             const std::function<void(const std::error_code& ec, const TopicRouteDataPtr& ptr)>& cb) {
        std::error_code ec;
        cb(ec, topic_route_data_);
      };

  EXPECT_CALL(*client_manager_, resolveRoute)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_resolve_route));

  bool cb_invoked = false;
  SendResult send_result;
  auto mock_send = [&](const std::string& target_host, const Metadata& metadata, SendMessageRequest& request,
                       SendCallback* cb) {
    cb->onSuccess(send_result);
    cb_invoked = true;
    return true;
  };

  EXPECT_CALL(*client_manager_, send).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(mock_send));
  producer_->start();

  MQMessage message(topic_, tag_, message_body_);
  std::error_code ec;
  producer_->send(message, ec);
  EXPECT_FALSE(ec);
  EXPECT_TRUE(cb_invoked);
  producer_->shutdown();
}

TEST_F(ProducerImplTest, testSend_WithMessageGroup) {
  auto mock_resolve_route =
      [this](const std::string& target_host, const Metadata& metadata, const QueryRouteRequest& request,
             std::chrono::milliseconds timeout,
             const std::function<void(const std::error_code& ec, const TopicRouteDataPtr& ptr)>& cb) {
        std::error_code ec;
        cb(ec, topic_route_data_);
      };

  EXPECT_CALL(*client_manager_, resolveRoute)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_resolve_route));

  bool cb_invoked = false;
  SendResult send_result;
  auto mock_send = [&](const std::string& target_host, const Metadata& metadata, SendMessageRequest& request,
                       SendCallback* cb) {
    cb->onSuccess(send_result);
    cb_invoked = true;
    return true;
  };

  EXPECT_CALL(*client_manager_, send).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(mock_send));
  producer_->start();

  MQMessage message(topic_, tag_, message_body_);
  message.bindMessageGroup(message_group_);
  std::error_code ec;
  producer_->send(message, ec);
  EXPECT_FALSE(ec);
  EXPECT_TRUE(cb_invoked);
  producer_->shutdown();
}

TEST_F(ProducerImplTest, testSend_WithMessageQueueSelector) {
  auto mock_resolve_route =
      [this](const std::string& target_host, const Metadata& metadata, const QueryRouteRequest& request,
             std::chrono::milliseconds timeout,
             const std::function<void(const std::error_code& ec, const TopicRouteDataPtr& ptr)>& cb) {
        std::error_code ec;
        cb(ec, topic_route_data_);
      };

  EXPECT_CALL(*client_manager_, resolveRoute)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_resolve_route));

  bool cb_invoked = false;
  SendResult send_result;
  auto mock_send = [&](const std::string& target_host, const Metadata& metadata, SendMessageRequest& request,
                       SendCallback* cb) {
    cb->onSuccess(send_result);
    cb_invoked = true;
    return true;
  };

  EXPECT_CALL(*client_manager_, send).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(mock_send));
  producer_->start();

  MQMessage message(topic_, tag_, message_body_);

  std::error_code ec;
  auto&& list = producer_->listMessageQueue(topic_, ec);

  EXPECT_FALSE(list.empty());

  message.bindMessageQueue(list[0]);

  producer_->send(message, ec);

  EXPECT_TRUE(cb_invoked);
  producer_->shutdown();
}

class TestSendCallback : public SendCallback {
public:
  TestSendCallback(bool& completed, absl::Mutex& mtx, absl::CondVar& cv) : completed_(completed), mtx_(mtx), cv_(cv) {
  }
  void onSuccess(SendResult& send_result) noexcept override {
    absl::MutexLock lk(&mtx_);
    completed_ = true;
    cv_.SignalAll();
  }

  void onFailure(const std::error_code& ec) noexcept override {
    absl::MutexLock lk(&mtx_);
    completed_ = true;
    cv_.SignalAll();
  }

protected:
  bool& completed_;
  absl::Mutex& mtx_;
  absl::CondVar& cv_;
};

TEST_F(ProducerImplTest, testAsyncSend) {
  auto mock_resolve_route =
      [this](const std::string& target_host, const Metadata& metadata, const QueryRouteRequest& request,
             std::chrono::milliseconds timeout,
             const std::function<void(const std::error_code& ec, const TopicRouteDataPtr& ptr)>& cb) {
        std::error_code ec;
        cb(ec, topic_route_data_);
      };

  EXPECT_CALL(*client_manager_, resolveRoute)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_resolve_route));

  bool cb_invoked = false;
  SendResult send_result;
  auto mock_send = [&](const std::string& target_host, const Metadata& metadata, SendMessageRequest& request,
                       SendCallback* cb) {
    cb->onSuccess(send_result);
    cb_invoked = true;
    return true;
  };

  EXPECT_CALL(*client_manager_, send).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(mock_send));

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  auto send_callback = absl::make_unique<TestSendCallback>(completed, mtx, cv);

  producer_->start();

  MQMessage message(topic_, tag_, message_body_);
  producer_->send(message, send_callback.get());

  if (!completed) {
    absl::MutexLock lk(&mtx);
    cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
  }

  EXPECT_TRUE(cb_invoked);
  producer_->shutdown();
}

ROCKETMQ_NAMESPACE_END