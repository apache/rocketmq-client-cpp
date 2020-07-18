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
#include "MQMessage.h"

#include <algorithm>  // std::move

#include "UtilAll.h"  // null
#include "MessageImpl.h"

namespace rocketmq {

MQMessage::MQMessage() : MQMessage(null, null) {}

MQMessage::MQMessage(const std::string& topic, const std::string& body) : MQMessage(topic, null, body) {}

MQMessage::MQMessage(const std::string& topic, const std::string& tags, const std::string& body)
    : MQMessage(topic, tags, null, body) {}

MQMessage::MQMessage(const std::string& topic,
                     const std::string& tags,
                     const std::string& keys,
                     const std::string& body)
    : MQMessage(topic, tags, keys, 0, body, true) {}

MQMessage::MQMessage(const std::string& topic,
                     const std::string& tags,
                     const std::string& keys,
                     int32_t flag,
                     const std::string& body,
                     bool waitStoreMsgOK)
    : message_impl_(std::make_shared<MessageImpl>(topic, tags, keys, flag, body, waitStoreMsgOK)) {}

MQMessage::~MQMessage() = default;

const std::string& MQMessage::getProperty(const std::string& name) const {
  return message_impl_->getProperty(name);
}

void MQMessage::putProperty(const std::string& name, const std::string& value) {
  message_impl_->putProperty(name, value);
}

void MQMessage::clearProperty(const std::string& name) {
  message_impl_->clearProperty(name);
}

const std::string& MQMessage::getTopic() const {
  return message_impl_->getTopic();
}

void MQMessage::setTopic(const std::string& topic) {
  message_impl_->setTopic(topic);
}

void MQMessage::setTopic(const char* body, int len) {
  message_impl_->setTopic(body, len);
}

const std::string& MQMessage::getTags() const {
  return message_impl_->getTags();
}

void MQMessage::setTags(const std::string& tags) {
  message_impl_->setTags(tags);
}

const std::string& MQMessage::getKeys() const {
  return message_impl_->getKeys();
}

void MQMessage::setKeys(const std::string& keys) {
  message_impl_->setKeys(keys);
}

void MQMessage::setKeys(const std::vector<std::string>& keys) {
  message_impl_->setKeys(keys);
}

int MQMessage::getDelayTimeLevel() const {
  return message_impl_->getDelayTimeLevel();
}

void MQMessage::setDelayTimeLevel(int level) {
  message_impl_->setDelayTimeLevel(level);
}

bool MQMessage::isWaitStoreMsgOK() const {
  return message_impl_->isWaitStoreMsgOK();
}

void MQMessage::setWaitStoreMsgOK(bool waitStoreMsgOK) {
  message_impl_->setWaitStoreMsgOK(waitStoreMsgOK);
}

int32_t MQMessage::getFlag() const {
  return message_impl_->getFlag();
}

void MQMessage::setFlag(int32_t flag) {
  message_impl_->setFlag(flag);
}

const std::string& MQMessage::getBody() const {
  return message_impl_->getBody();
}

void MQMessage::setBody(const char* body, int len) {
  message_impl_->setBody(body, len);
}

void MQMessage::setBody(const std::string& body) {
  message_impl_->setBody(body);
}

void MQMessage::setBody(std::string&& body) {
  message_impl_->setBody(std::move(body));
}

const std::string& MQMessage::getTransactionId() const {
  return message_impl_->getTransactionId();
}

void MQMessage::setTransactionId(const std::string& transactionId) {
  message_impl_->setTransactionId(transactionId);
}

const std::map<std::string, std::string>& MQMessage::getProperties() const {
  return message_impl_->getProperties();
}

void MQMessage::setProperties(const std::map<std::string, std::string>& properties) {
  message_impl_->setProperties(properties);
}

void MQMessage::setProperties(std::map<std::string, std::string>&& properties) {
  message_impl_->setProperties(std::move(properties));
}

bool MQMessage::isBatch() const {
  return message_impl_->isBatch();
}

std::string MQMessage::toString() const {
  return message_impl_->toString();
}

MessagePtr MQMessage::getMessageImpl() {
  return message_impl_;
}

}  // namespace rocketmq
