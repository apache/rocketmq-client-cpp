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
#include "DefaultLitePullConsumer.h"

#include "DefaultLitePullConsumerConfigImpl.hpp"
#include "DefaultLitePullConsumerImpl.h"
#include "UtilAll.h"

namespace rocketmq {

DefaultLitePullConsumer::DefaultLitePullConsumer(const std::string& groupname)
    : DefaultLitePullConsumer(groupname, nullptr) {}

DefaultLitePullConsumer::DefaultLitePullConsumer(const std::string& groupname, RPCHookPtr rpcHook)
    : DefaultLitePullConsumerConfigProxy(std::make_shared<DefaultLitePullConsumerConfigImpl>()),
      pull_consumer_impl_(nullptr) {
  // set default group name
  if (groupname.empty()) {
    set_group_name(DEFAULT_CONSUMER_GROUP);
  } else {
    set_group_name(groupname);
  }

  // create DefaultLitePullConsumerImpl
  pull_consumer_impl_ = DefaultLitePullConsumerImpl::create(real_config(), rpcHook);
}

DefaultLitePullConsumer::~DefaultLitePullConsumer() = default;

void DefaultLitePullConsumer::start() {
  pull_consumer_impl_->start();
}

void DefaultLitePullConsumer::shutdown() {
  pull_consumer_impl_->shutdown();
}

bool DefaultLitePullConsumer::isAutoCommit() const {
  return pull_consumer_impl_->isAutoCommit();
}

void DefaultLitePullConsumer::setAutoCommit(bool auto_commit) {
  pull_consumer_impl_->setAutoCommit(auto_commit);
}

void DefaultLitePullConsumer::subscribe(const std::string& topic, const std::string& subExpression) {
  pull_consumer_impl_->subscribe(topic, subExpression);
}

void DefaultLitePullConsumer::subscribe(const std::string& topic, const MessageSelector& selector) {
  pull_consumer_impl_->subscribe(topic, selector);
}

void DefaultLitePullConsumer::unsubscribe(const std::string& topic) {
  pull_consumer_impl_->unsubscribe(topic);
}

std::vector<MQMessageExt> DefaultLitePullConsumer::poll() {
  return pull_consumer_impl_->poll();
}

std::vector<MQMessageExt> DefaultLitePullConsumer::poll(long timeout) {
  return pull_consumer_impl_->poll(timeout);
}

std::vector<MQMessageQueue> DefaultLitePullConsumer::fetchMessageQueues(const std::string& topic) {
  return pull_consumer_impl_->fetchMessageQueues(topic);
}

void DefaultLitePullConsumer::assign(const std::vector<MQMessageQueue>& messageQueues) {
  pull_consumer_impl_->assign(messageQueues);
}

void DefaultLitePullConsumer::seek(const MQMessageQueue& messageQueue, int64_t offset) {
  pull_consumer_impl_->seek(messageQueue, offset);
}

void DefaultLitePullConsumer::seekToBegin(const MQMessageQueue& messageQueue) {
  pull_consumer_impl_->seekToBegin(messageQueue);
}

void DefaultLitePullConsumer::seekToEnd(const MQMessageQueue& messageQueue) {
  pull_consumer_impl_->seekToEnd(messageQueue);
}

int64_t DefaultLitePullConsumer::offsetForTimestamp(const MQMessageQueue& messageQueue, int64_t timestamp) {
  return pull_consumer_impl_->offsetForTimestamp(messageQueue, timestamp);
}

void DefaultLitePullConsumer::pause(const std::vector<MQMessageQueue>& messageQueues) {
  pull_consumer_impl_->pause(messageQueues);
}

void DefaultLitePullConsumer::resume(const std::vector<MQMessageQueue>& messageQueues) {
  pull_consumer_impl_->resume(messageQueues);
}

void DefaultLitePullConsumer::commitSync() {
  pull_consumer_impl_->commitSync();
}

int64_t DefaultLitePullConsumer::committed(const MQMessageQueue& messageQueue) {
  return pull_consumer_impl_->committed(messageQueue);
}

void DefaultLitePullConsumer::registerTopicMessageQueueChangeListener(
    const std::string& topic,
    TopicMessageQueueChangeListener* topicMessageQueueChangeListener) {
  pull_consumer_impl_->registerTopicMessageQueueChangeListener(topic, topicMessageQueueChangeListener);
}

void DefaultLitePullConsumer::setRPCHook(RPCHookPtr rpcHook) {
  dynamic_cast<DefaultLitePullConsumerImpl*>(pull_consumer_impl_.get())->setRPCHook(rpcHook);
}

}  // namespace rocketmq
