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
#include "DefaultMQPullConsumer.h"

#include "UtilAll.h"

namespace rocketmq {

DefaultMQPullConsumer::DefaultMQPullConsumer(const std::string& groupname)
    : DefaultMQPullConsumer(groupname, nullptr) {}

DefaultMQPullConsumer::DefaultMQPullConsumer(const std::string& groupname, std::shared_ptr<RPCHook> rpcHook)
    : DefaultMQPullConsumerConfigProxy(nullptr), m_pullConsumerDelegate(nullptr) {
  // set default group name
  if (groupname.empty()) {
    setGroupName(DEFAULT_CONSUMER_GROUP);
  } else {
    setGroupName(groupname);
  }

  // TODO: DefaultMQPullConsumerImpl
}

DefaultMQPullConsumer::~DefaultMQPullConsumer() = default;

void DefaultMQPullConsumer::start() {
  m_pullConsumerDelegate->start();
}

void DefaultMQPullConsumer::shutdown() {
  m_pullConsumerDelegate->shutdown();
}

bool DefaultMQPullConsumer::sendMessageBack(MQMessageExt& msg, int delayLevel) {
  return m_pullConsumerDelegate->sendMessageBack(msg, delayLevel);
}

bool DefaultMQPullConsumer::sendMessageBack(MQMessageExt& msg, int delayLevel, const std::string& brokerName) {
  return m_pullConsumerDelegate->sendMessageBack(msg, delayLevel, brokerName);
}

void DefaultMQPullConsumer::fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) {
  m_pullConsumerDelegate->fetchMessageQueuesInBalance(topic, mqs);
}

void DefaultMQPullConsumer::registerMessageQueueListener(const std::string& topic, MQueueListener* listener) {
  m_pullConsumerDelegate->registerMessageQueueListener(topic, listener);
}

PullResult DefaultMQPullConsumer::pull(const MQMessageQueue& mq,
                                       const std::string& subExpression,
                                       int64_t offset,
                                       int maxNums) {
  return m_pullConsumerDelegate->pull(mq, subExpression, offset, maxNums);
}

void DefaultMQPullConsumer::pull(const MQMessageQueue& mq,
                                 const std::string& subExpression,
                                 int64_t offset,
                                 int maxNums,
                                 PullCallback* pullCallback) {
  m_pullConsumerDelegate->pull(mq, subExpression, offset, maxNums, pullCallback);
}

PullResult DefaultMQPullConsumer::pullBlockIfNotFound(const MQMessageQueue& mq,
                                                      const std::string& subExpression,
                                                      int64_t offset,
                                                      int maxNums) {
  return m_pullConsumerDelegate->pullBlockIfNotFound(mq, subExpression, offset, maxNums);
}

void DefaultMQPullConsumer::pullBlockIfNotFound(const MQMessageQueue& mq,
                                                const std::string& subExpression,
                                                int64_t offset,
                                                int maxNums,
                                                PullCallback* pullCallback) {
  m_pullConsumerDelegate->pullBlockIfNotFound(mq, subExpression, offset, maxNums, pullCallback);
}

void DefaultMQPullConsumer::updateConsumeOffset(const MQMessageQueue& mq, int64_t offset) {
  m_pullConsumerDelegate->updateConsumeOffset(mq, offset);
}

int64_t DefaultMQPullConsumer::fetchConsumeOffset(const MQMessageQueue& mq, bool fromStore) {
  return m_pullConsumerDelegate->fetchConsumeOffset(mq, fromStore);
}

void DefaultMQPullConsumer::fetchMessageQueuesInBalance(const std::string& topic, std::vector<MQMessageQueue>& mqs) {
  m_pullConsumerDelegate->fetchMessageQueuesInBalance(topic, mqs);
}

void DefaultMQPullConsumer::setRPCHook(std::shared_ptr<RPCHook> rpcHook) {
  // dynamic_cast<DefaultMQPullConsumerImpl*>(m_pullConsumerDelegate.get())->setRPCHook(rpcHook);
}

}  // namespace rocketmq
