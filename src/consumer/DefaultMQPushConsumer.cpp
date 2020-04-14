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

#include "DefaultMQPushConsumerConfigImpl.h"
#include "DefaultMQPushConsumerImpl.h"
#include "UtilAll.h"

namespace rocketmq {

DefaultMQPushConsumer::DefaultMQPushConsumer(const std::string& groupname)
    : DefaultMQPushConsumer(groupname, nullptr) {}

DefaultMQPushConsumer::DefaultMQPushConsumer(const std::string& groupname, RPCHookPtr rpcHook)
    : DefaultMQPushConsumerConfigProxy(std::make_shared<DefaultMQPushConsumerConfigImpl>()),
      m_pushConsumerDelegate(nullptr) {
  // set default group name
  if (groupname.empty()) {
    setGroupName(DEFAULT_CONSUMER_GROUP);
  } else {
    setGroupName(groupname);
  }

  m_pushConsumerDelegate = DefaultMQPushConsumerImpl::create(getRealConfig(), rpcHook);
}

DefaultMQPushConsumer::~DefaultMQPushConsumer() = default;

void DefaultMQPushConsumer::start() {
  m_pushConsumerDelegate->start();
}

void DefaultMQPushConsumer::shutdown() {
  m_pushConsumerDelegate->shutdown();
}

bool DefaultMQPushConsumer::sendMessageBack(MQMessageExt& msg, int delayLevel) {
  return m_pushConsumerDelegate->sendMessageBack(msg, delayLevel);
}

bool DefaultMQPushConsumer::sendMessageBack(MQMessageExt& msg, int delayLevel, const std::string& brokerName) {
  return m_pushConsumerDelegate->sendMessageBack(msg, delayLevel, brokerName);
}

void DefaultMQPushConsumer::fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) {
  m_pushConsumerDelegate->fetchSubscribeMessageQueues(topic, mqs);
}

void DefaultMQPushConsumer::registerMessageListener(MQMessageListener* messageListener) {
  m_pushConsumerDelegate->registerMessageListener(messageListener);
}

void DefaultMQPushConsumer::registerMessageListener(MessageListenerConcurrently* messageListener) {
  m_pushConsumerDelegate->registerMessageListener(messageListener);
}

void DefaultMQPushConsumer::registerMessageListener(MessageListenerOrderly* messageListener) {
  m_pushConsumerDelegate->registerMessageListener(messageListener);
}

MQMessageListener* DefaultMQPushConsumer::getMessageListener() const {
  return m_pushConsumerDelegate->getMessageListener();
}

void DefaultMQPushConsumer::subscribe(const std::string& topic, const std::string& subExpression) {
  m_pushConsumerDelegate->subscribe(topic, subExpression);
}

void DefaultMQPushConsumer::suspend() {
  m_pushConsumerDelegate->suspend();
}

void DefaultMQPushConsumer::resume() {
  m_pushConsumerDelegate->resume();
}

void DefaultMQPushConsumer::setRPCHook(RPCHookPtr rpcHook) {
  std::dynamic_pointer_cast<DefaultMQPushConsumerImpl>(m_pushConsumerDelegate)->setRPCHook(rpcHook);
}

}  // namespace rocketmq
