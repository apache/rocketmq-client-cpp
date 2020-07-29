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

bool operator==(std::nullptr_t, const MQMessage& message) noexcept {
  return nullptr == message.message_impl_;
}

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

const std::string& MQMessage::topic() const {
  return message_impl_->topic();
}

void MQMessage::set_topic(const std::string& topic) {
  message_impl_->set_topic(topic);
}

void MQMessage::set_topic(const char* body, int len) {
  message_impl_->set_topic(body, len);
}

const std::string& MQMessage::tags() const {
  return message_impl_->tags();
}

void MQMessage::set_tags(const std::string& tags) {
  message_impl_->set_tags(tags);
}

const std::string& MQMessage::keys() const {
  return message_impl_->keys();
}

void MQMessage::set_keys(const std::string& keys) {
  message_impl_->set_keys(keys);
}

void MQMessage::set_keys(const std::vector<std::string>& keys) {
  message_impl_->set_keys(keys);
}

int MQMessage::delay_time_level() const {
  return message_impl_->delay_time_level();
}

void MQMessage::set_delay_time_level(int level) {
  message_impl_->set_delay_time_level(level);
}

bool MQMessage::wait_store_msg_ok() const {
  return message_impl_->wait_store_msg_ok();
}

void MQMessage::set_wait_store_msg_ok(bool waitStoreMsgOK) {
  message_impl_->set_wait_store_msg_ok(waitStoreMsgOK);
}

int32_t MQMessage::flag() const {
  return message_impl_->flag();
}

void MQMessage::set_flag(int32_t flag) {
  message_impl_->set_flag(flag);
}

const std::string& MQMessage::body() const {
  return message_impl_->body();
}

void MQMessage::set_body(const std::string& body) {
  message_impl_->set_body(body);
}

void MQMessage::set_body(std::string&& body) {
  message_impl_->set_body(std::move(body));
}

const std::string& MQMessage::transaction_id() const {
  return message_impl_->transaction_id();
}

void MQMessage::set_transaction_id(const std::string& transactionId) {
  message_impl_->set_transaction_id(transactionId);
}

const std::map<std::string, std::string>& MQMessage::properties() const {
  return message_impl_->properties();
}

void MQMessage::set_properties(const std::map<std::string, std::string>& properties) {
  message_impl_->set_properties(properties);
}

void MQMessage::set_properties(std::map<std::string, std::string>&& properties) {
  message_impl_->set_properties(std::move(properties));
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
