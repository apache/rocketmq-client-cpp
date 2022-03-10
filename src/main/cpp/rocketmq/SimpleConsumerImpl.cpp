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
#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "absl/time/time.h"

#include "Assignment.h"
#include "ClientManager.h"
#include "FilterExpression.h"
#include "LoggerImpl.h"
#include "MessageAccessor.h"
#include "RpcClient.h"
#include "Signature.h"
#include "SimpleConsumerImpl.h"
#include "SimpleReceiveMessageCallback.h"
#include "TopicAssignmentInfo.h"
#include "apache/rocketmq/v1/definition.pb.h"
#include "rocketmq/ExpressionType.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

SimpleConsumerImpl::SimpleConsumerImpl(const std::string& group_name)
    : ClientImpl(group_name), group_name_(group_name) {
}

void SimpleConsumerImpl::start() {
  ClientImpl::start();

  State expected = State::STARTING;
  if (state_.compare_exchange_strong(expected, State::STARTED)) {
    // Start services of this class
    // Check configuration
    std::vector<std::string> topics;
    {
      absl::MutexLock lk(&topic_filter_expression_table_mtx_);
      for (const auto& entry : topic_filter_expression_table_) {
        topics.insert(topics.end(), entry.first);
      }
    }

    if (!topics.empty()) {
      std::vector<std::string>::size_type total = topics.size();
      absl::Mutex mtx;
      absl::CondVar cv;

      auto callback = [&](const std::error_code& ec, TopicRouteDataPtr) {
        absl::MutexLock lk(&mtx);
        if (!--total) {
          cv.SignalAll();
        }
      };

      for (const auto& topic : topics) {
        this->getRouteFor(topic, callback);
      }

      {
        absl::MutexLock lk(&mtx);
        cv.Wait(&mtx);
      }
    } else {
      SPDLOG_WARN("Need to subscribe one or more toipics before invoking start()");
    }

    // Heartbeat.await
    heartbeat0(true);

    // ScanAssignment.await
    {
      std::size_t countdown = topics.size();
      absl::Mutex mtx;
      absl::CondVar cv;

      auto callback = [&, this](const std::error_code& ec, const TopicAssignmentPtr& assignments) {
        if (ec) {
          SPDLOG_WARN("Failed to queryAssignment. Cause: {}", ec.message());
        } else if (assignments && !assignments->assignmentList().empty()) {
          absl::MutexLock lk(&topic_assignment_map_mtx_);
          std::string topic = assignments->assignmentList()[0].messageQueue().getTopic();
          topic_assignment_map_.insert_or_assign(topic, assignments);
          SPDLOG_INFO("Acquired aassignments for topic={}", topic);
        } else {
          SPDLOG_WARN("Got an invalid assignment result");
        }

        {
          absl::MutexLock lk(&mtx);
          if (!--countdown) {
            cv.SignalAll();
          }
        }
      };

      for (const auto& topic : topics) {
        queryAssignment(topic, callback);
      }

      {
        absl::MutexLock lk(&mtx);
        cv.Wait(&mtx);
      }
    }

    // Scan assignments periodically
    std::weak_ptr<SimpleConsumerImpl> consumer(shared_from_this());
    auto scan_assignment_task = std::bind(&SimpleConsumerImpl::scanAssignments, consumer);
    scheduleAtFixDelay("Scan-Assignment-Task", scan_assignment_task, std::chrono::seconds(3), std::chrono::seconds(10));
    SPDLOG_INFO("SimpleConsumerImpl#start() completed");
  }
}

void SimpleConsumerImpl::shutdown() {
  // Shutdown services of this class

  State expected = State::STARTED;
  if (state_.compare_exchange_strong(expected, State::STOPPING)) {
    ClientImpl::shutdown();
  }
}

void SimpleConsumerImpl::scanAssignments(std::weak_ptr<SimpleConsumerImpl> consumer) {
  auto simple_consumer = consumer.lock();
  if (!simple_consumer) {
    return;
  }

  simple_consumer->scanAssignments0();
}

void SimpleConsumerImpl::scanAssignments0() {

  absl::flat_hash_set<std::string> topics;
  {
    absl::MutexLock lk(&topic_route_table_mtx_);
    for (const auto& entry : topic_route_table_) {
      topics.insert(entry.first);
    }
  }

  std::weak_ptr<SimpleConsumerImpl> consumer(shared_from_this());
  auto callback = std::bind(&SimpleConsumerImpl::onAssignments, consumer, std::placeholders::_1, std::placeholders::_2);
  for (const std::string& topic : topics) {
    queryAssignment(topic, callback);
  }
}

void SimpleConsumerImpl::onAssignments(std::weak_ptr<SimpleConsumerImpl> consumer, const std::error_code& ec,
                                       const TopicAssignmentPtr& assignments) {
  std::shared_ptr<SimpleConsumerImpl> simple_consumer = consumer.lock();
  if (!simple_consumer) {
    return;
  }

  simple_consumer->onAssignments0(ec, assignments);
}

void SimpleConsumerImpl::onAssignments0(const std::error_code& ec, const TopicAssignmentPtr& assignments) {
  if (ec) {
    SPDLOG_WARN("Failed to query topic assignment. Cause: {}", ec.message());
    return;
  }

  if (!assignments) {
    SPDLOG_ERROR("Unexpected nullptr assignment results. Skip");
    return;
  }

  if (assignments->assignmentList().empty()) {
    SPDLOG_WARN("Got empty assignment list. Skip");
    return;
  }

  std::string topic = assignments->assignmentList()[0].messageQueue().getTopic();

  TopicAssignmentPtr existing;

  {
    absl::MutexLock lk(&topic_assignment_map_mtx_);
    topic_assignment_map_.insert_or_assign(topic, assignments);
  }
}

void SimpleConsumerImpl::queryAssignment(
    const std::string& topic, const std::function<void(const std::error_code&, const TopicAssignmentPtr&)>& cb) {

  auto callback = [this, topic, cb](const std::error_code& ec, const TopicRouteDataPtr& topic_route) {
    TopicAssignmentPtr topic_assignment;
    if (MessageModel::BROADCASTING == message_model_) {
      if (ec) {
        SPDLOG_WARN("Failed to get valid route entries for topic={}. Cause: {}", topic, ec.message());
        cb(ec, topic_assignment);
      }

      std::vector<Assignment> assignments;
      assignments.reserve(topic_route->partitions().size());
      for (const auto& partition : topic_route->partitions()) {
        assignments.emplace_back(Assignment(partition.asMessageQueue()));
      }
      topic_assignment = std::make_shared<TopicAssignment>(std::move(assignments));
      cb(ec, topic_assignment);
      return;
    }

    std::string broker_host;
    if (!selectBroker(topic_route, broker_host)) {
      SPDLOG_WARN("Failed to select a broker to query assignment for group={}, topic={}", group_name_, topic);
    }

    QueryAssignmentRequest request;
    setAccessPoint(request.mutable_endpoints());
    wrapQueryAssignmentRequest(topic, group_name_, clientId(), MixAll::DEFAULT_LOAD_BALANCER_STRATEGY_NAME_, request);
    SPDLOG_DEBUG("QueryAssignmentRequest: {}", request.DebugString());

    absl::flat_hash_map<std::string, std::string> metadata;
    Signature::sign(this, metadata);
    auto assignment_callback = [this, cb, topic, broker_host](const std::error_code& ec,
                                                              const QueryAssignmentResponse& response) {
      if (ec) {
        SPDLOG_WARN("Failed to acquire queue assignment of topic={} from brokerAddress={}", topic, broker_host);
        cb(ec, nullptr);
      } else {
        SPDLOG_DEBUG("Query topic assignment OK. Topic={}, group={}, assignment-size={}", topic, group_name_,
                     response.assignments().size());
        SPDLOG_TRACE("Query assignment response for {} is: {}", topic, response.DebugString());
        cb(ec, std::make_shared<TopicAssignment>(response));
      }
    };

    client_manager_->queryAssignment(broker_host, metadata, request, absl::ToChronoMilliseconds(io_timeout_),
                                     assignment_callback);
  };
  getRouteFor(topic, callback);
}

void SimpleConsumerImpl::prepareHeartbeatData(HeartbeatRequest& request) {
  request.set_client_id(clientId());
  auto consumer_data = request.mutable_consumer_data();
  consumer_data->mutable_group()->set_resource_namespace(resourceNamespace());
  consumer_data->mutable_group()->set_name(group_name_);
  auto subscriptions = consumer_data->mutable_subscriptions();
  {
    absl::MutexLock lk(&topic_filter_expression_table_mtx_);
    for (const auto& entry : topic_filter_expression_table_) {
      auto subscription = new rmq::SubscriptionEntry();
      subscription->mutable_topic()->set_resource_namespace(resourceNamespace());
      subscription->mutable_topic()->set_name(entry.first);

      subscription->mutable_expression()->set_expression(entry.second.content_);
      switch (entry.second.type_) {
        case ExpressionType::TAG: {
          subscription->mutable_expression()->set_type(rmq::FilterType::TAG);
          break;
        }
        case ExpressionType::SQL92: {
          subscription->mutable_expression()->set_type(rmq::FilterType::SQL);
          break;
        }
      }
      subscriptions->AddAllocated(subscription);
    }
  }
  consumer_data->set_consume_model(rmq::ConsumeModel::CLUSTERING);
  consumer_data->set_consume_policy(rmq::ConsumePolicy::RESUME);
  consumer_data->set_consume_type(rmq::ConsumeMessageType::ACTIVE);
  request.set_fifo_flag(false);
}

void SimpleConsumerImpl::subscribe(const std::string& topic, const std::string& expression,
                                   ExpressionType expression_type) {
  FilterExpression filter_expression(topic, expression_type);
  {
    absl::MutexLock lk(&topic_filter_expression_table_mtx_);
    topic_filter_expression_table_.insert_or_assign(topic, filter_expression);
  }
}

std::vector<MQMessageExt> SimpleConsumerImpl::receive(const std::string topic, std::chrono::seconds invisible_duration,
                                                      std::error_code& ec, std::size_t max_number_of_messages,
                                                      std::chrono::seconds await_duration) {
  TopicAssignmentPtr assignments;
  {
    absl::MutexLock lk(&topic_assignment_map_mtx_);
    if (!topic_assignment_map_.contains(topic)) {
      SPDLOG_WARN("No assignments available for {}", topic);
      return {};
    }
    assignments = topic_assignment_map_[topic];
  }

  if (!assignments || assignments->assignmentList().empty()) {
    SPDLOG_WARN("No assignments available for {}", topic);
    return {};
  }

  const Assignment& assignment = assignments->nextAssignment();

  ReceiveMessageRequest request;
  request.mutable_await_time()->set_seconds(await_duration.count());
  request.mutable_await_time()->set_nanos(0);

  request.set_client_id(clientId());
  request.mutable_group()->set_resource_namespace(resourceNamespace());
  request.mutable_group()->set_name(group_name_);

  request.mutable_invisible_duration()->set_seconds(invisible_duration.count());
  request.mutable_invisible_duration()->set_nanos(0);

  {
    absl::MutexLock lk(&topic_filter_expression_table_mtx_);
    if (!topic_filter_expression_table_.contains(topic)) {
      const auto& expression = topic_filter_expression_table_.at(topic);
      request.mutable_filter_expression()->set_expression(expression.content_);
      switch (expression.type_) {
        case ExpressionType::TAG: {
          request.mutable_filter_expression()->set_type(rmq::FilterType::TAG);
        }
        case ExpressionType::SQL92: {
          request.mutable_filter_expression()->set_type(rmq::FilterType::SQL);
        }
      }
    }
  }

  request.mutable_partition()->mutable_topic()->set_resource_namespace(resourceNamespace());
  request.mutable_partition()->mutable_topic()->set_name(topic);

  request.mutable_partition()->set_id(assignment.messageQueue().getQueueId());

  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(this, metadata);

  absl::CondVar cv;
  absl::Mutex mtx;
  bool compeleted = false;

  std::vector<MQMessageExt> messages;

  auto callback = [&](const std::error_code& ec, const ReceiveMessageResult& result) {
    messages.insert(messages.begin(), result.messages.begin(), result.messages.end());
    {
      absl::MutexLock lk(&mtx);
      compeleted = true;
      cv.SignalAll();
    }
  };

  auto receive_message_callback = std::make_shared<SimpleReceiveMessageCallback>(callback);

  client_manager_->receiveMessage(assignment.messageQueue().serviceAddress(), metadata, request,
                                  absl::ToChronoMilliseconds(getLongPollingTimeout()), receive_message_callback);

  {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
    SPDLOG_DEBUG("Should have got ReceiveMessageResponse from {}", assignment.messageQueue().serviceAddress());
  }

  return messages;
}

void SimpleConsumerImpl::ack(const MQMessageExt& message, std::function<void(const std::error_code&)> callback) {
  AckMessageRequest request;
  request.mutable_topic()->set_resource_namespace(resourceNamespace());
  request.mutable_topic()->set_name(message.getTopic());

  request.mutable_group()->set_resource_namespace(resourceNamespace());
  request.mutable_group()->set_name(group_name_);

  request.set_message_id(message.getMsgId());
  request.set_receipt_handle(message.receiptHandle());
  request.set_client_id(clientId());

  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(this, metadata);

  client_manager_->ack(MessageAccessor::targetEndpoint(message), metadata, request,
                       absl::ToChronoMilliseconds(getIoTimeout()), callback);
}

void SimpleConsumerImpl::changeInvisibleDuration(
    const MQMessageExt& message, std::chrono::seconds invisible_duration,
    std::function<void(const std::error_code&)> callback) {
  ChangeInvisibleDurationRequest request;
  request.mutable_group()->set_resource_namespace(resourceNamespace());
  request.mutable_group()->set_name(group_name_);

  request.mutable_topic()->set_resource_namespace(resourceNamespace());
  request.mutable_topic()->set_name(message.getTopic());

  request.set_receipt_handle(message.receiptHandle());

  request.mutable_invisible_duration()->set_seconds(invisible_duration.count());
  request.mutable_invisible_duration()->set_nanos(0);


  Metadata metadata;
  Signature::sign(this, metadata);

  client_manager_->changeInvisibleDuration(MessageAccessor::targetEndpoint(message), metadata, request,
                                           absl::ToChronoMilliseconds(getIoTimeout()),
                                           callback);
}

ROCKETMQ_NAMESPACE_END