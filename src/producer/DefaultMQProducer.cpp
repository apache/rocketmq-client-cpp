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

#include "DefaultMQProducerConfigImpl.hpp"
#include "DefaultMQProducerImpl.h"
#include "UtilAll.h"

namespace rocketmq {

DefaultMQProducer::DefaultMQProducer(const std::string& groupname) : DefaultMQProducer(groupname, nullptr) {}

DefaultMQProducer::DefaultMQProducer(const std::string& groupname, RPCHookPtr rpcHook)
    : DefaultMQProducer(groupname, rpcHook, std::make_shared<DefaultMQProducerConfigImpl>()) {}

DefaultMQProducer::DefaultMQProducer(const std::string& groupname,
                                     RPCHookPtr rpcHook,
                                     DefaultMQProducerConfigPtr producerConfig)
    : DefaultMQProducerConfigProxy(producerConfig), producer_impl_(nullptr) {
  // set default group name
  if (groupname.empty()) {
    set_group_name(DEFAULT_PRODUCER_GROUP);
  } else {
    set_group_name(groupname);
  }

  // create DefaultMQProducerImpl
  producer_impl_ = DefaultMQProducerImpl::create(real_config(), rpcHook);
}

DefaultMQProducer::~DefaultMQProducer() = default;

void DefaultMQProducer::start() {
  producer_impl_->start();
}

void DefaultMQProducer::shutdown() {
  producer_impl_->shutdown();
}

std::vector<MQMessageQueue> DefaultMQProducer::fetchPublishMessageQueues(const std::string& topic) {
  return producer_impl_->fetchPublishMessageQueues(topic);
}

SendResult DefaultMQProducer::send(MQMessage& msg) {
  return producer_impl_->send(msg);
}

SendResult DefaultMQProducer::send(MQMessage& msg, long timeout) {
  return producer_impl_->send(msg, timeout);
}

SendResult DefaultMQProducer::send(MQMessage& msg, const MQMessageQueue& mq) {
  return producer_impl_->send(msg, mq);
}

SendResult DefaultMQProducer::send(MQMessage& msg, const MQMessageQueue& mq, long timeout) {
  return producer_impl_->send(msg, mq, timeout);
}

void DefaultMQProducer::send(MQMessage& msg, SendCallback* sendCallback) noexcept {
  producer_impl_->send(msg, sendCallback, send_msg_timeout());
}

void DefaultMQProducer::send(MQMessage& msg, SendCallback* sendCallback, long timeout) noexcept {
  producer_impl_->send(msg, sendCallback, timeout);
}

void DefaultMQProducer::send(MQMessage& msg, const MQMessageQueue& mq, SendCallback* sendCallback) noexcept {
  return producer_impl_->send(msg, mq, sendCallback);
}

void DefaultMQProducer::send(MQMessage& msg,
                             const MQMessageQueue& mq,
                             SendCallback* sendCallback,
                             long timeout) noexcept {
  producer_impl_->send(msg, mq, sendCallback, timeout);
}

void DefaultMQProducer::sendOneway(MQMessage& msg) {
  producer_impl_->sendOneway(msg);
}

void DefaultMQProducer::sendOneway(MQMessage& msg, const MQMessageQueue& mq) {
  producer_impl_->sendOneway(msg, mq);
}

SendResult DefaultMQProducer::send(MQMessage& msg, MessageQueueSelector* selector, void* arg) {
  return producer_impl_->send(msg, selector, arg);
}

SendResult DefaultMQProducer::send(MQMessage& msg, MessageQueueSelector* selector, void* arg, long timeout) {
  return producer_impl_->send(msg, selector, arg, timeout);
}

void DefaultMQProducer::send(MQMessage& msg,
                             MessageQueueSelector* selector,
                             void* arg,
                             SendCallback* sendCallback) noexcept {
  return producer_impl_->send(msg, selector, arg, sendCallback);
}

void DefaultMQProducer::send(MQMessage& msg,
                             MessageQueueSelector* selector,
                             void* arg,
                             SendCallback* sendCallback,
                             long timeout) noexcept {
  producer_impl_->send(msg, selector, arg, sendCallback, timeout);
}

void DefaultMQProducer::sendOneway(MQMessage& msg, MessageQueueSelector* selector, void* arg) {
  producer_impl_->sendOneway(msg, selector, arg);
}

TransactionSendResult DefaultMQProducer::sendMessageInTransaction(MQMessage& msg, void* arg) {
  THROW_MQEXCEPTION(MQClientException, "sendMessageInTransaction not implement, please use TransactionMQProducer class",
                    -1);
}

SendResult DefaultMQProducer::send(std::vector<MQMessage>& msgs) {
  return producer_impl_->send(msgs);
}

SendResult DefaultMQProducer::send(std::vector<MQMessage>& msgs, long timeout) {
  return producer_impl_->send(msgs, timeout);
}

SendResult DefaultMQProducer::send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq) {
  return producer_impl_->send(msgs, mq);
}

SendResult DefaultMQProducer::send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq, long timeout) {
  return producer_impl_->send(msgs, mq, timeout);
}

void DefaultMQProducer::send(std::vector<MQMessage>& msgs, SendCallback* sendCallback) {
  producer_impl_->send(msgs, sendCallback);
}

void DefaultMQProducer::send(std::vector<MQMessage>& msgs, SendCallback* sendCallback, long timeout) {
  producer_impl_->send(msgs, sendCallback, timeout);
}

void DefaultMQProducer::send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq, SendCallback* sendCallback) {
  producer_impl_->send(msgs, mq, sendCallback);
}

void DefaultMQProducer::send(std::vector<MQMessage>& msgs,
                             const MQMessageQueue& mq,
                             SendCallback* sendCallback,
                             long timeout) {
  producer_impl_->send(msgs, mq, sendCallback, timeout);
}

MQMessage DefaultMQProducer::request(MQMessage& msg, long timeout) {
  return producer_impl_->request(msg, timeout);
}

void DefaultMQProducer::request(MQMessage& msg, RequestCallback* requestCallback, long timeout) {
  producer_impl_->request(msg, requestCallback, timeout);
}

MQMessage DefaultMQProducer::request(MQMessage& msg, const MQMessageQueue& mq, long timeout) {
  return producer_impl_->request(msg, mq, timeout);
}

void DefaultMQProducer::request(MQMessage& msg,
                                const MQMessageQueue& mq,
                                RequestCallback* requestCallback,
                                long timeout) {
  producer_impl_->request(msg, mq, requestCallback, timeout);
}

MQMessage DefaultMQProducer::request(MQMessage& msg, MessageQueueSelector* selector, void* arg, long timeout) {
  return producer_impl_->request(msg, selector, arg, timeout);
}

void DefaultMQProducer::request(MQMessage& msg,
                                MessageQueueSelector* selector,
                                void* arg,
                                RequestCallback* requestCallback,
                                long timeout) {
  producer_impl_->request(msg, selector, arg, requestCallback, timeout);
}

bool DefaultMQProducer::send_latency_fault_enable() const {
  return dynamic_cast<DefaultMQProducerImpl*>(producer_impl_.get())->isSendLatencyFaultEnable();
}

void DefaultMQProducer::set_send_latency_fault_enable(bool sendLatencyFaultEnable) {
  dynamic_cast<DefaultMQProducerImpl*>(producer_impl_.get())->setSendLatencyFaultEnable(sendLatencyFaultEnable);
}

void DefaultMQProducer::setRPCHook(RPCHookPtr rpcHook) {
  dynamic_cast<DefaultMQProducerImpl*>(producer_impl_.get())->setRPCHook(rpcHook);
}

}  // namespace rocketmq
