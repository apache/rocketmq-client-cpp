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

#include "gtest/gtest.h"
#include <system_error>

#include "ClientManagerFactory.h"
#include "ClientManagerMock.h"
#include "InvocationContext.h"
#include "MessageAccessor.h"
#include "PushConsumerImpl.h"
#include "Scheduler.h"
#include "StaticNameServerResolver.h"
#include "grpc/grpc.h"
#include "rocketmq/MQMessageExt.h"
#include "rocketmq/MessageListener.h"

ROCKETMQ_NAMESPACE_BEGIN

class TestStandardMessageListener : public StandardMessageListener {
public:
  ConsumeMessageResult consumeMessage(const std::vector<MQMessageExt>& msgs) override {
    return ConsumeMessageResult::SUCCESS;
  }
};

class PushConsumerImplTest : public testing::Test {
public:
  PushConsumerImplTest() : message_listener_(absl::make_unique<TestStandardMessageListener>()) {
  }

  void SetUp() override {
    grpc_init();
    scheduler_ = std::make_shared<SchedulerImpl>();
    name_server_resolver_ = std::make_shared<StaticNameServerResolver>(name_server_list_);
    client_manager_ = std::make_shared<testing::NiceMock<ClientManagerMock>>();
    ClientManagerFactory::getInstance().addClientManager(resource_namespace_, client_manager_);
    push_consumer_ = std::make_shared<PushConsumerImpl>(group_);
    push_consumer_->resourceNamespace(resource_namespace_);
    push_consumer_->withNameServerResolver(name_server_resolver_);
    push_consumer_->registerMessageListener(message_listener_.get());
    scheduler_->start();

    ON_CALL(*client_manager_, getScheduler).WillByDefault(testing::Return(scheduler_));
  }

  void TearDown() override {
    scheduler_->shutdown();
    grpc_shutdown();
  }

protected:
  SchedulerSharedPtr scheduler_;
  std::string name_server_list_{"10.0.0.1:9876"};
  std::shared_ptr<StaticNameServerResolver> name_server_resolver_;
  std::string resource_namespace_{"mq://test"};
  std::string group_{"CID_test"};
  std::string topic_{"Topic0"};
  std::string tag_{"TagA"};
  std::string key_{"key-0"};
  std::string message_body_{"Message Body Content"};
  int delay_level_{1};
  std::shared_ptr<testing::NiceMock<ClientManagerMock>> client_manager_;
  std::unique_ptr<MessageListener> message_listener_;
  std::shared_ptr<PushConsumerImpl> push_consumer_;
  const std::string target_endpoint_{"localhost:10911"};
};

TEST_F(PushConsumerImplTest, testAck) {
  auto ack_cb = [](const std::string& target_host, const Metadata& metadata, const AckMessageRequest& request,
                   std::chrono::milliseconds timeout, const std::function<void(const std::error_code&)>& cb) {
    std::error_code ec;
    cb(ec);
  };

  EXPECT_CALL(*client_manager_, ack).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(ack_cb));

  push_consumer_->start();

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;
  auto callback = [&](const std::error_code& ec) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
  };

  MQMessageExt message;
  message.setTopic(topic_);
  message.setBody(message_body_);
  message.setTags(tag_);
  message.setKey(key_);
  message.setDelayTimeLevel(delay_level_);
  MessageAccessor::setTargetEndpoint(message, target_endpoint_);

  push_consumer_->ack(message, callback);

  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);
  push_consumer_->shutdown();
}

TEST_F(PushConsumerImplTest, testNack) {
  auto nack_cb = [](const std::string& target_host, const Metadata& metadata, const NackMessageRequest& request,
                    std::chrono::milliseconds timeout, const std::function<void(const std::error_code&)>& cb) {
    std::error_code ec;
    cb(ec);
  };

  EXPECT_CALL(*client_manager_, nack).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(nack_cb));

  push_consumer_->start();

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;
  auto callback = [&](const std::error_code& ec) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
  };

  MQMessageExt message;
  message.setTopic(topic_);
  message.setBody(message_body_);
  message.setTags(tag_);
  message.setKey(key_);
  message.setDelayTimeLevel(delay_level_);
  MessageAccessor::setTargetEndpoint(message, target_endpoint_);
  push_consumer_->nack(message, callback);
  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);
  push_consumer_->shutdown();
}

TEST_F(PushConsumerImplTest, testForward) {
  InvocationContext<ForwardMessageToDeadLetterQueueResponse> invocation_context;

  auto forward_cb =
      [&](const std::string& target_host, const Metadata& metadata,
          const ForwardMessageToDeadLetterQueueRequest& request, std::chrono::milliseconds timeout,
          const std::function<void(const InvocationContext<ForwardMessageToDeadLetterQueueResponse>*)>& cb) {
        cb(&invocation_context);
      };

  EXPECT_CALL(*client_manager_, forwardMessageToDeadLetterQueue)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(forward_cb));

  push_consumer_->start();

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;
  auto callback = [&](bool ok) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
  };

  MQMessageExt message;
  message.setTopic(topic_);
  message.setBody(message_body_);
  message.setTags(tag_);
  message.setKey(key_);
  message.setDelayTimeLevel(delay_level_);

  push_consumer_->forwardToDeadLetterQueue(message, callback);
  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);
  push_consumer_->shutdown();
}

ROCKETMQ_NAMESPACE_END