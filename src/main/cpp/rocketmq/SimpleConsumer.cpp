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

#include "SimpleConsumerImpl.h"
#include "StaticNameServerResolver.h"
#include "rocketmq/SimpleConsumer.h"

ROCKETMQ_NAMESPACE_BEGIN

SimpleConsumer::SimpleConsumer(const std::string& group_name)
    : group_name_(group_name), simple_consumer_impl_(std::make_shared<SimpleConsumerImpl>(group_name)) {
}

SimpleConsumer::~SimpleConsumer() {
  simple_consumer_impl_->shutdown();
}

void SimpleConsumer::setCredentialsProvider(std::shared_ptr<CredentialsProvider> provider) {
  simple_consumer_impl_->setCredentialsProvider(std::move(provider));
}

void SimpleConsumer::setResourceNamespace(const std::string& resource_namespace) {
  simple_consumer_impl_->resourceNamespace(resource_namespace);
}

void SimpleConsumer::setInstanceName(const std::string& instance_name) {
  simple_consumer_impl_->setInstanceName(instance_name);
}

void SimpleConsumer::setNamesrvAddr(const std::string& name_srv) {
  auto name_server_resolver = std::make_shared<StaticNameServerResolver>(name_srv);
  simple_consumer_impl_->withNameServerResolver(name_server_resolver);
}

void SimpleConsumer::start() {
  simple_consumer_impl_->start();
}

void SimpleConsumer::subscribe(const std::string& topic, const std::string& expression,
                               ExpressionType expression_type) {
  simple_consumer_impl_->subscribe(topic, expression, expression_type);
}

std::vector<MQMessageExt> SimpleConsumer::receive(const std::string topic, std::chrono::seconds invisible_duration,
                                                  std::error_code& ec, std::size_t max_number_of_messages,
                                                  std::chrono::seconds await_duration) {
  return simple_consumer_impl_->receive(topic, invisible_duration, ec, max_number_of_messages, await_duration);
}

void SimpleConsumer::ack(const MQMessageExt& message, std::function<void(const std::error_code&)> callback) {
  simple_consumer_impl_->ack(message, callback);
}

void SimpleConsumer::changeInvisibleDuration(const MQMessageExt& message, std::chrono::seconds invisible_duration,
                                             std::function<void(const std::error_code&)> callback) {
  return simple_consumer_impl_->changeInvisibleDuration(message, invisible_duration, callback);
}

ROCKETMQ_NAMESPACE_END