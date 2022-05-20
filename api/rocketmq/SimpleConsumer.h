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

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "Configuration.h"
#include "FilterExpression.h"
#include "Message.h"
#include "RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

using ReceiveCallback = std::function<void(const std::error_code&, const std::vector<MessageConstSharedPtr>&)>;

using AckCallback = std::function<void(const std::error_code&)>;

using ChangeInvisibleDurationCallback = std::function<void(const std::error_code&)>;

class SimpleConsumerImpl;

class SimpleConsumerBuilder;

class SimpleConsumer {
public:
  ~SimpleConsumer();

  static SimpleConsumerBuilder newBuilder();

  void subscribe(std::string topic, FilterExpression filter_expression);

  void unsubscribe(const std::string& topic);

  void receive(std::size_t limit,
               std::chrono::milliseconds invisible_duration,
               std::error_code& ec,
               std::vector<MessageConstSharedPtr>& messages);

  void asyncReceive(std::size_t limit, std::chrono::milliseconds invisible_duration, ReceiveCallback callback);

  void ack(const Message& message, std::error_code& ec);

  void asyncAck(const Message& message, AckCallback callback);

  void changeInvisibleDuration(const Message& message, std::chrono::milliseconds duration, std::error_code& ec);

  void asyncChangeInvisibleDuration(const Message& message,
                                    std::chrono::milliseconds duration,
                                    ChangeInvisibleDurationCallback callback);

private:
  std::shared_ptr<SimpleConsumerImpl> impl_;

  SimpleConsumer(std::string group);

  void start();

  friend class SimpleConsumerBuilder;
};

class SimpleConsumerBuilder {
public:
  SimpleConsumerBuilder();

  SimpleConsumerBuilder& withGroup(std::string group) {
    group_ = std::move(group);
    return *this;
  }

  SimpleConsumerBuilder& subscribe(std::string topic, FilterExpression expression) {
    subscriptions_.insert({std::move(topic), std::move(expression)});
    return *this;
  }

  SimpleConsumerBuilder& withConfiguration(Configuration configuration) {
    configuration_ = std::move(configuration);
    return *this;
  }

  SimpleConsumer build();

private:
  // Group name the consumer belongs to
  std::string group_;

  Configuration configuration_;

  std::unordered_map<std::string, FilterExpression> subscriptions_;
};

ROCKETMQ_NAMESPACE_END