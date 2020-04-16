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
#include "DefaultMQProducer.h"

#include "DefaultMQProducerConfigImpl.h"
#include "DefaultMQProducerImpl.h"
#include "UtilAll.h"

namespace rocketmq {

DefaultMQProducer::DefaultMQProducer(const std::string& groupname) : DefaultMQProducer(groupname, nullptr) {}

DefaultMQProducer::DefaultMQProducer(const std::string& groupname, RPCHookPtr rpcHook)
    : DefaultMQProducer(groupname, rpcHook, std::make_shared<DefaultMQProducerConfigImpl>()) {}

DefaultMQProducer::DefaultMQProducer(const std::string& groupname,
                                     RPCHookPtr rpcHook,
                                     DefaultMQProducerConfigPtr producerConfig)
    : DefaultMQProducerConfigProxy(producerConfig), m_producerDelegate(nullptr) {
  // set default group name
  if (groupname.empty()) {
    setGroupName(DEFAULT_PRODUCER_GROUP);
  } else {
    setGroupName(groupname);
  }

  m_producerDelegate = DefaultMQProducerImpl::create(getRealConfig(), rpcHook);
}

DefaultMQProducer::~DefaultMQProducer() = default;

void DefaultMQProducer::start() {
  m_producerDelegate->start();
}

void DefaultMQProducer::shutdown() {
  m_producerDelegate->shutdown();
}

SendResult DefaultMQProducer::send(MQMessagePtr msg) {
  return m_producerDelegate->send(msg);
}

SendResult DefaultMQProducer::send(MQMessagePtr msg, long timeout) {
  return m_producerDelegate->send(msg, timeout);
}

SendResult DefaultMQProducer::send(MQMessagePtr msg, const MQMessageQueue& mq) {
  return m_producerDelegate->send(msg, mq);
}

SendResult DefaultMQProducer::send(MQMessagePtr msg, const MQMessageQueue& mq, long timeout) {
  return m_producerDelegate->send(msg, mq, timeout);
}

void DefaultMQProducer::send(MQMessagePtr msg, SendCallback* sendCallback) noexcept {
  m_producerDelegate->send(msg, sendCallback, getSendMsgTimeout());
}

void DefaultMQProducer::send(MQMessagePtr msg, SendCallback* sendCallback, long timeout) noexcept {
  m_producerDelegate->send(msg, sendCallback, timeout);
}

void DefaultMQProducer::send(MQMessagePtr msg, const MQMessageQueue& mq, SendCallback* sendCallback) noexcept {
  return m_producerDelegate->send(msg, mq, sendCallback);
}

void DefaultMQProducer::send(MQMessagePtr msg,
                             const MQMessageQueue& mq,
                             SendCallback* sendCallback,
                             long timeout) noexcept {
  m_producerDelegate->send(msg, mq, sendCallback, timeout);
}

void DefaultMQProducer::sendOneway(MQMessagePtr msg) {
  m_producerDelegate->sendOneway(msg);
}

void DefaultMQProducer::sendOneway(MQMessagePtr msg, const MQMessageQueue& mq) {
  m_producerDelegate->sendOneway(msg, mq);
}

SendResult DefaultMQProducer::send(MQMessagePtr msg, MessageQueueSelector* selector, void* arg) {
  return m_producerDelegate->send(msg, selector, arg);
}

SendResult DefaultMQProducer::send(MQMessagePtr msg, MessageQueueSelector* selector, void* arg, long timeout) {
  return m_producerDelegate->send(msg, selector, arg, timeout);
}

void DefaultMQProducer::send(MQMessagePtr msg,
                             MessageQueueSelector* selector,
                             void* arg,
                             SendCallback* sendCallback) noexcept {
  return m_producerDelegate->send(msg, selector, arg, sendCallback);
}

void DefaultMQProducer::send(MQMessagePtr msg,
                             MessageQueueSelector* selector,
                             void* arg,
                             SendCallback* sendCallback,
                             long timeout) noexcept {
  m_producerDelegate->send(msg, selector, arg, sendCallback, timeout);
}

void DefaultMQProducer::sendOneway(MQMessagePtr msg, MessageQueueSelector* selector, void* arg) {
  m_producerDelegate->sendOneway(msg, selector, arg);
}

TransactionSendResult DefaultMQProducer::sendMessageInTransaction(MQMessagePtr msg, void* arg) {
  THROW_MQEXCEPTION(MQClientException, "sendMessageInTransaction not implement, please use TransactionMQProducer class",
                    -1);
}

SendResult DefaultMQProducer::send(std::vector<MQMessagePtr>& msgs) {
  return m_producerDelegate->send(msgs);
}

SendResult DefaultMQProducer::send(std::vector<MQMessagePtr>& msgs, long timeout) {
  return m_producerDelegate->send(msgs, timeout);
}

SendResult DefaultMQProducer::send(std::vector<MQMessagePtr>& msgs, const MQMessageQueue& mq) {
  return m_producerDelegate->send(msgs, mq);
}

SendResult DefaultMQProducer::send(std::vector<MQMessagePtr>& msgs, const MQMessageQueue& mq, long timeout) {
  return m_producerDelegate->send(msgs, mq, timeout);
}

MQMessagePtr DefaultMQProducer::request(MQMessagePtr msg, long timeout) {
  return m_producerDelegate->request(msg, timeout);
}

bool DefaultMQProducer::isSendLatencyFaultEnable() const {
  return std::dynamic_pointer_cast<DefaultMQProducerImpl>(m_producerDelegate)->isSendLatencyFaultEnable();
}

void DefaultMQProducer::setSendLatencyFaultEnable(bool sendLatencyFaultEnable) {
  std::dynamic_pointer_cast<DefaultMQProducerImpl>(m_producerDelegate)
      ->setSendLatencyFaultEnable(sendLatencyFaultEnable);
}

void DefaultMQProducer::setRPCHook(RPCHookPtr rpcHook) {
  std::dynamic_pointer_cast<DefaultMQProducerImpl>(m_producerDelegate)->setRPCHook(rpcHook);
}

}  // namespace rocketmq
