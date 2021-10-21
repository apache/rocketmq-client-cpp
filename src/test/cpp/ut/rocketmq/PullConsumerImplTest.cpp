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
#include <system_error>

#include "ClientManagerFactory.h"
#include "ClientManagerMock.h"
#include "InvocationContext.h"
#include "PullConsumerImpl.h"
#include "Scheduler.h"
#include "StaticNameServerResolver.h"
#include "apache/rocketmq/v1/definition.pb.h"
#include "rocketmq/AsyncCallback.h"
#include "rocketmq/ConsumeType.h"
#include "rocketmq/ErrorCode.h"
#include "rocketmq/MQMessageExt.h"
#include "rocketmq/RocketMQ.h"

#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class PullConsumerImplTest : public testing::Test {
public:
  void SetUp() override {
    grpc_init();
    scheduler_ = std::make_shared<SchedulerImpl>();
    name_server_resolver_ = std::make_shared<StaticNameServerResolver>(name_server_list_);

    scheduler_->start();
    client_manager_ = std::make_shared<testing::NiceMock<ClientManagerMock>>();
    ON_CALL(*client_manager_, getScheduler).WillByDefault(testing::Return(scheduler_));
    ClientManagerFactory::getInstance().addClientManager(resource_namespace_, client_manager_);

    pull_consumer_ = std::make_shared<PullConsumerImpl>(group_);
    pull_consumer_->withNameServerResolver(name_server_resolver_);
    pull_consumer_->resourceNamespace(resource_namespace_);

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
    grpc_shutdown();
    scheduler_->shutdown();
  }

protected:
  std::string resource_namespace_{"mq://test"};
  std::string name_server_list_{"10.0.0.1:9876"};
  std::shared_ptr<NameServerResolver> name_server_resolver_;
  std::string group_{"Group-0"};
  std::string topic_{"Test"};
  std::string tag_{"TagB"};
  std::shared_ptr<testing::NiceMock<ClientManagerMock>> client_manager_;
  std::shared_ptr<PullConsumerImpl> pull_consumer_;
  SchedulerSharedPtr scheduler_;
  std::string broker_name_{"broker-a"};
  int broker_id_{0};
  std::string message_body_{"Message Body Content"};
  std::string broker_host_{"10.0.0.1"};
  int broker_port_{10911};
  int queue_id_{1};
  TopicRouteDataPtr topic_route_data_;
  int batch_size_{32};
};

TEST_F(PullConsumerImplTest, testStartShutdown) {
  pull_consumer_->start();
  pull_consumer_->shutdown();
}

TEST_F(PullConsumerImplTest, testQueuesFor) {
  pull_consumer_->start();
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

  std::future<std::vector<MQMessageQueue>> future = pull_consumer_->queuesFor(topic_);
  auto queues = future.get();
  EXPECT_FALSE(queues.empty());

  pull_consumer_->queuesFor(topic_);

  pull_consumer_->shutdown();
}

class TestPullCallback : public PullCallback {
public:
  TestPullCallback(bool& success, bool& failure) : success_(success), failure_(failure) {
  }
  void onSuccess(const PullResult& pull_result) noexcept override {
    success_ = true;
    failure_ = false;
  }

  void onFailure(const std::error_code& e) noexcept override {
    failure_ = true;
    success_ = false;
  }

private:
  bool& success_;
  bool& failure_;
};

TEST_F(PullConsumerImplTest, testPull) {
  pull_consumer_->start();
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

  std::error_code ec;
  ReceiveMessageResult result;

  for (int i = 0; i < batch_size_; i++) {
    MQMessageExt message;
    message.setBody(message_body_);
    message.setTopic(topic_);
    message.setTags(tag_);
    result.messages.emplace_back(message);
  }

  auto mock_pull_message = [&](const std::string& target_host, const Metadata& metadata,
                               const PullMessageRequest& request, std::chrono::milliseconds timeout,
                               const std::function<void(const std::error_code&, const ReceiveMessageResult&)>& cb) {
    cb(ec, result);
  };

  EXPECT_CALL(*client_manager_, pullMessage)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_pull_message));

  std::future<std::vector<MQMessageQueue>> future = pull_consumer_->queuesFor(topic_);
  auto queues = future.get();
  EXPECT_FALSE(queues.empty());

  PullMessageQuery query;
  query.message_queue = *queues.begin();
  query.offset = 0;
  query.await_time = std::chrono::seconds(3);

  bool success = false;
  bool failure = false;
  auto pull_callback = new TestPullCallback(success, failure);
  pull_consumer_->pull(query, pull_callback);

  EXPECT_TRUE(success);
  EXPECT_FALSE(failure);

  pull_consumer_->shutdown();
  delete pull_callback;
}

TEST_F(PullConsumerImplTest, testPull_gRPC_error) {
  pull_consumer_->start();
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

  std::error_code ec;
  ReceiveMessageResult result;
  auto mock_pull_message = [&](const std::string& target_host, const Metadata& metadata,
                               const PullMessageRequest& request, std::chrono::milliseconds timeout,
                               const std::function<void(const std::error_code&, const ReceiveMessageResult&)>& cb) {
    ec = ErrorCode::BadRequest;
    cb(ec, result);
  };

  EXPECT_CALL(*client_manager_, pullMessage)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_pull_message));

  std::future<std::vector<MQMessageQueue>> future = pull_consumer_->queuesFor(topic_);
  auto queues = future.get();
  EXPECT_FALSE(queues.empty());

  PullMessageQuery query;
  query.message_queue = *queues.begin();
  query.offset = 0;
  query.await_time = std::chrono::seconds(3);

  bool success = false;
  bool failure = false;
  auto pull_callback = new TestPullCallback(success, failure);
  pull_consumer_->pull(query, pull_callback);

  EXPECT_TRUE(failure);
  EXPECT_FALSE(success);

  pull_consumer_->shutdown();
  delete pull_callback;
}

TEST_F(PullConsumerImplTest, testPull_biz_error) {
  pull_consumer_->start();
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

  std::error_code ec;
  ReceiveMessageResult result;

  auto mock_pull_message = [&](const std::string& target_host, const Metadata& metadata,
                               const PullMessageRequest& request, std::chrono::milliseconds timeout,
                               const std::function<void(const std::error_code&, const ReceiveMessageResult&)>& cb) {
    ec = ErrorCode::BadRequest;
    cb(ec, result);
  };

  EXPECT_CALL(*client_manager_, pullMessage)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_pull_message));

  std::future<std::vector<MQMessageQueue>> future = pull_consumer_->queuesFor(topic_);
  auto queues = future.get();
  EXPECT_FALSE(queues.empty());

  PullMessageQuery query;
  query.message_queue = *queues.begin();
  query.offset = 0;
  query.await_time = std::chrono::seconds(3);

  bool success = false;
  bool failure = false;
  auto pull_callback = new TestPullCallback(success, failure);
  pull_consumer_->pull(query, pull_callback);

  EXPECT_FALSE(success);
  EXPECT_TRUE(failure);

  pull_consumer_->shutdown();
  delete pull_callback;
}

TEST_F(PullConsumerImplTest, testQueryOffset) {
  pull_consumer_->start();
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

  std::future<std::vector<MQMessageQueue>> future = pull_consumer_->queuesFor(topic_);
  auto queues = future.get();
  EXPECT_FALSE(queues.empty());

  QueryOffsetResponse response;
  int64_t offset = 1;
  response.set_offset(offset);

  auto mock_query_offset = [&](const std::string& target_host, const Metadata& metadata,
                               const QueryOffsetRequest& request, std::chrono::milliseconds timeout,
                               const std::function<void(const std::error_code&, const QueryOffsetResponse&)>& cb) {
    std::error_code ec;
    cb(ec, response);
  };

  EXPECT_CALL(*client_manager_, queryOffset)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_query_offset));

  OffsetQuery query;
  query.policy = QueryOffsetPolicy::BEGINNING;
  query.message_queue = *queues.begin();

  std::future<int64_t> offset_future = pull_consumer_->queryOffset(query);
  EXPECT_EQ(offset, offset_future.get());

  pull_consumer_->shutdown();
}

TEST_F(PullConsumerImplTest, testQueryOffset_End) {
  pull_consumer_->start();
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

  std::future<std::vector<MQMessageQueue>> future = pull_consumer_->queuesFor(topic_);
  auto queues = future.get();
  EXPECT_FALSE(queues.empty());

  QueryOffsetResponse response;
  int64_t offset = 1;
  response.set_offset(offset);

  auto mock_query_offset = [&](const std::string& target_host, const Metadata& metadata,
                               const QueryOffsetRequest& request, std::chrono::milliseconds timeout,
                               const std::function<void(const std::error_code&, const QueryOffsetResponse&)>& cb) {
    std::error_code ec;
    cb(ec, response);
  };

  EXPECT_CALL(*client_manager_, queryOffset)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_query_offset));

  OffsetQuery query;
  query.policy = QueryOffsetPolicy::END;
  query.message_queue = *queues.begin();

  std::future<int64_t> offset_future = pull_consumer_->queryOffset(query);
  EXPECT_EQ(offset, offset_future.get());

  pull_consumer_->shutdown();
}

TEST_F(PullConsumerImplTest, testQueryOffset_Timepoint) {
  pull_consumer_->start();
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

  std::future<std::vector<MQMessageQueue>> future = pull_consumer_->queuesFor(topic_);
  auto queues = future.get();
  EXPECT_FALSE(queues.empty());

  QueryOffsetResponse response;
  int64_t offset = 1;
  response.set_offset(offset);

  auto mock_query_offset = [&](const std::string& target_host, const Metadata& metadata,
                               const QueryOffsetRequest& request, std::chrono::milliseconds timeout,
                               const std::function<void(const std::error_code&, const QueryOffsetResponse&)>& cb) {
    std::error_code ec;
    cb(ec, response);
  };

  EXPECT_CALL(*client_manager_, queryOffset)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_query_offset));

  OffsetQuery query;
  query.policy = QueryOffsetPolicy::TIME_POINT;
  query.time_point = std::chrono::system_clock::now();
  query.message_queue = *queues.begin();

  std::future<int64_t> offset_future = pull_consumer_->queryOffset(query);
  EXPECT_EQ(offset, offset_future.get());

  pull_consumer_->shutdown();
}

ROCKETMQ_NAMESPACE_END