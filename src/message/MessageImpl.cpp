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
#include "MessageImpl.h"

#include <algorithm>  // std::move
#include <sstream>    // std::stringstream

#include "MQMessageConst.h"
#include "MessageSysFlag.h"
#include "UtilAll.h"

namespace rocketmq {

MessageImpl::MessageImpl() : MessageImpl(null, null) {}

MessageImpl::MessageImpl(const std::string& topic, const std::string& body)
    : MessageImpl(topic, null, null, 0, body, true) {}

MessageImpl::MessageImpl(const std::string& topic,
                         const std::string& tags,
                         const std::string& keys,
                         int32_t flag,
                         const std::string& body,
                         bool waitStoreMsgOK)
    : topic_(topic), flag_(flag), body_(body) {
  if (tags.length() > 0) {
    set_tags(tags);
  }

  if (keys.length() > 0) {
    set_keys(keys);
  }

  set_wait_store_msg_ok(waitStoreMsgOK);
}

MessageImpl::~MessageImpl() = default;

const std::string& MessageImpl::topic() const {
  return topic_;
}

void MessageImpl::set_topic(const std::string& topic) {
  topic_ = topic;
}

void MessageImpl::set_topic(const char* topic, int len) {
  topic_.clear();
  topic_.append(topic, len);
}

const std::string& MessageImpl::tags() const {
  return getProperty(MQMessageConst::PROPERTY_TAGS);
}

void MessageImpl::set_tags(const std::string& tags) {
  putProperty(MQMessageConst::PROPERTY_TAGS, tags);
}

const std::string& MessageImpl::keys() const {
  return getProperty(MQMessageConst::PROPERTY_KEYS);
}

void MessageImpl::set_keys(const std::string& keys) {
  putProperty(MQMessageConst::PROPERTY_KEYS, keys);
}

void MessageImpl::set_keys(const std::vector<std::string>& keys) {
  if (keys.empty()) {
    return;
  }

  std::string strKeys;
  auto it = keys.begin();
  strKeys += *it;
  for (it++; it != keys.end(); it++) {
    strKeys += MQMessageConst::KEY_SEPARATOR;
    strKeys += *it;
  }

  set_keys(strKeys);
}

int MessageImpl::delay_time_level() const {
  std::string tmp = getProperty(MQMessageConst::PROPERTY_DELAY_TIME_LEVEL);
  if (!tmp.empty()) {
    return atoi(tmp.c_str());
  }
  return 0;
}

void MessageImpl::set_delay_time_level(int level) {
  putProperty(MQMessageConst::PROPERTY_DELAY_TIME_LEVEL, UtilAll::to_string(level));
}

bool MessageImpl::wait_store_msg_ok() const {
  std::string tmp = getProperty(MQMessageConst::PROPERTY_WAIT_STORE_MSG_OK);
  return tmp.empty() || UtilAll::stob(tmp);
}

void MessageImpl::set_wait_store_msg_ok(bool waitStoreMsgOK) {
  putProperty(MQMessageConst::PROPERTY_WAIT_STORE_MSG_OK, UtilAll::to_string(waitStoreMsgOK));
}

int32_t MessageImpl::flag() const {
  return flag_;
}

void MessageImpl::set_flag(int32_t flag) {
  flag_ = flag;
}

const std::string& MessageImpl::body() const {
  return body_;
}

void MessageImpl::set_body(const std::string& body) {
  body_ = body;
}

void MessageImpl::set_body(std::string&& body) {
  body_ = std::move(body);
}

const std::string& MessageImpl::transaction_id() const {
  return transaction_id_;
}

void MessageImpl::set_transaction_id(const std::string& transactionId) {
  transaction_id_ = transactionId;
}

const std::map<std::string, std::string>& MessageImpl::properties() const {
  return properties_;
}

void MessageImpl::set_properties(const std::map<std::string, std::string>& properties) {
  properties_ = properties;
}

void MessageImpl::set_properties(std::map<std::string, std::string>&& properties) {
  properties_ = std::move(properties);
}

const std::string& MessageImpl::getProperty(const std::string& name) const {
  const auto& it = properties_.find(name);
  if (it != properties_.end()) {
    return it->second;
  }
  return null;
}

void MessageImpl::putProperty(const std::string& name, const std::string& value) {
  properties_[name] = value;
}

void MessageImpl::clearProperty(const std::string& name) {
  properties_.erase(name);
}

std::string MessageImpl::toString() const {
  std::stringstream ss;
  ss << "Message [topic=" << topic_ << ", flag=" << flag_ << ", tag=" << tags() << ", transactionId='"
     << transaction_id_ + "']";
  return ss.str();
}

}  // namespace rocketmq
