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
#include "DefaultMQPushConsumer.h"

#include "DefaultMQPushConsumerConfigImpl.hpp"
#include "DefaultMQPushConsumerImpl.h"
#include "UtilAll.h"

namespace rocketmq {

DefaultMQPushConsumer::DefaultMQPushConsumer(const std::string& groupname)
    : DefaultMQPushConsumer(groupname, nullptr) {}

DefaultMQPushConsumer::DefaultMQPushConsumer(const std::string& groupname, RPCHookPtr rpcHook)
    : DefaultMQPushConsumerConfigProxy(std::make_shared<DefaultMQPushConsumerConfigImpl>()),
      push_consumer_impl_(nullptr) {
  // set default group name
  if (groupname.empty()) {
    set_group_name(DEFAULT_CONSUMER_GROUP);
  } else {
    set_group_name(groupname);
  }

  // create DefaultMQPushConsumerImpl
  push_consumer_impl_ = DefaultMQPushConsumerImpl::create(real_config(), rpcHook);
}

DefaultMQPushConsumer::~DefaultMQPushConsumer() = default;

void DefaultMQPushConsumer::start() {
  push_consumer_impl_->start();
}

void DefaultMQPushConsumer::shutdown() {
  push_consumer_impl_->shutdown();
}

void DefaultMQPushConsumer::suspend() {
  push_consumer_impl_->suspend();
}

void DefaultMQPushConsumer::resume() {
  push_consumer_impl_->resume();
}

MQMessageListener* DefaultMQPushConsumer::getMessageListener() const {
  return push_consumer_impl_->getMessageListener();
}

void DefaultMQPushConsumer::registerMessageListener(MessageListenerConcurrently* messageListener) {
  push_consumer_impl_->registerMessageListener(messageListener);
}

void DefaultMQPushConsumer::registerMessageListener(MessageListenerOrderly* messageListener) {
  push_consumer_impl_->registerMessageListener(messageListener);
}

void DefaultMQPushConsumer::subscribe(const std::string& topic, const std::string& subExpression) {
  push_consumer_impl_->subscribe(topic, subExpression);
}

bool DefaultMQPushConsumer::sendMessageBack(MessageExtPtr msg, int delayLevel) {
  return push_consumer_impl_->sendMessageBack(msg, delayLevel);
}

bool DefaultMQPushConsumer::sendMessageBack(MessageExtPtr msg, int delayLevel, const std::string& brokerName) {
  return push_consumer_impl_->sendMessageBack(msg, delayLevel, brokerName);
}

void DefaultMQPushConsumer::setRPCHook(RPCHookPtr rpcHook) {
  dynamic_cast<DefaultMQPushConsumerImpl*>(push_consumer_impl_.get())->setRPCHook(rpcHook);
}

}  // namespace rocketmq
