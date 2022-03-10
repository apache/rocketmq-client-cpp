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
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include "ExpressionType.h"
#include "MQMessageExt.h"
#include "RocketMQ.h"
#include "rocketmq/CredentialsProvider.h"

ROCKETMQ_NAMESPACE_BEGIN

/**
 * This class employs pointer-to-implementation paradigm to achieve the goal of stable ABI.
 * Refer https://en.cppreference.com/w/cpp/language/pimpl for an explanation.
 */
class SimpleConsumerImpl;

class SimpleConsumer {
public:
  SimpleConsumer(const std::string& group_name);

  ~SimpleConsumer();

  void setCredentialsProvider(std::shared_ptr<CredentialsProvider>);

  void setResourceNamespace(const std::string&);

  void setInstanceName(const std::string&);

  void setNamesrvAddr(const std::string&);

  void start();

  void subscribe(const std::string& topic, const std::string& expression,
                 ExpressionType expression_type = ExpressionType::TAG);

  std::vector<MQMessageExt> receive(const std::string topic, std::chrono::seconds invisible_duration,
                                    std::error_code& ec, std::size_t max_number_of_messages = 32,
                                    std::chrono::seconds await_duration = std::chrono::seconds(30));

  void ack(const MQMessageExt& message, std::function<void(const std::error_code& ec)> callback);

  void changeInvisibleDuration(const MQMessageExt& message, std::chrono::seconds invisible_duration,
                               std::function<void(const std::error_code&)> callback);

  const std::string& groupName();

private:
  std::string group_name_;
  std::shared_ptr<SimpleConsumerImpl> simple_consumer_impl_;
};

ROCKETMQ_NAMESPACE_END