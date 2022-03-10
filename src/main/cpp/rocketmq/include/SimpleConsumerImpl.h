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
#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <system_error>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"

#include "ClientImpl.h"
#include "FilterExpression.h"
#include "RpcClient.h"
#include "TopicAssignmentInfo.h"
#include "rocketmq/ExpressionType.h"
#include "rocketmq/MessageModel.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class SimpleConsumerImpl : virtual public ClientImpl, public std::enable_shared_from_this<SimpleConsumerImpl> {
public:
  SimpleConsumerImpl(const std::string& group_name);

  void start() override;

  void shutdown() override;

  void prepareHeartbeatData(HeartbeatRequest& request) override;

  void subscribe(const std::string& topic, const std::string& expression, ExpressionType expression_type);

  std::vector<MQMessageExt> receive(const std::string topic, std::chrono::seconds invisible_duration,
                                    std::error_code& ec, std::size_t max_number_of_messages,
                                    std::chrono::seconds await_duration);

  void ack(const MQMessageExt& message, std::function<void(const std::error_code&)> callback);

  void changeInvisibleDuration(const MQMessageExt& message, std::chrono::seconds invisible_duration,
                               std::function<void(const std::error_code&)> callback);

protected:
  std::shared_ptr<ClientImpl> self() override {
    return shared_from_this();
  }

private:
  static void scanAssignments(std::weak_ptr<SimpleConsumerImpl> consumer);
  void scanAssignments0();

  /**
   * Query assignment of the specified topic from load balancer directly if
   * message consuming mode is clustering. In case current client is operating
   * in the broadcasting mode, assignments are constructed locally from topic
   * route entries.
   *
   * @param topic Topic to query
   * @return shared pointer to topic assignment info
   */
  void queryAssignment(const std::string& topic,
                       const std::function<void(const std::error_code&, const TopicAssignmentPtr&)>& cb);

  static void onAssignments(std::weak_ptr<SimpleConsumerImpl> consumer, const std::error_code& ec,
                            const TopicAssignmentPtr& assignments);
  void onAssignments0(const std::error_code& ec, const TopicAssignmentPtr& assignments);

  absl::flat_hash_map<std::string, FilterExpression>
      topic_filter_expression_table_ GUARDED_BY(topic_filter_expression_table_mtx_);
  absl::Mutex topic_filter_expression_table_mtx_;

  std::string group_name_;
  MessageModel message_model_{MessageModel::CLUSTERING};

  absl::flat_hash_map<std::string, TopicAssignmentPtr> topic_assignment_map_ GUARDED_BY(topic_assignment_map_mtx_);
  absl::Mutex topic_assignment_map_mtx_;

  std::atomic<std::size_t> receive_assignment_index_{0};
};

ROCKETMQ_NAMESPACE_END