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
#include "MessageSysFlag.h"
#include "UtilAll.h"

namespace rocketmq {

const std::string MQMessageConst::PROPERTY_KEYS = "KEYS";
const std::string MQMessageConst::PROPERTY_TAGS = "TAGS";
const std::string MQMessageConst::PROPERTY_WAIT_STORE_MSG_OK = "WAIT";
const std::string MQMessageConst::PROPERTY_DELAY_TIME_LEVEL = "DELAY";
const std::string MQMessageConst::PROPERTY_RETRY_TOPIC = "RETRY_TOPIC";
const std::string MQMessageConst::PROPERTY_REAL_TOPIC = "REAL_TOPIC";
const std::string MQMessageConst::PROPERTY_REAL_QUEUE_ID = "REAL_QID";
const std::string MQMessageConst::PROPERTY_TRANSACTION_PREPARED = "TRAN_MSG";
const std::string MQMessageConst::PROPERTY_PRODUCER_GROUP = "PGROUP";
const std::string MQMessageConst::PROPERTY_MIN_OFFSET = "MIN_OFFSET";
const std::string MQMessageConst::PROPERTY_MAX_OFFSET = "MAX_OFFSET";

const std::string MQMessageConst::PROPERTY_BUYER_ID = "BUYER_ID";
const std::string MQMessageConst::PROPERTY_ORIGIN_MESSAGE_ID = "ORIGIN_MESSAGE_ID";
const std::string MQMessageConst::PROPERTY_TRANSFER_FLAG = "TRANSFER_FLAG";
const std::string MQMessageConst::PROPERTY_CORRECTION_FLAG = "CORRECTION_FLAG";
const std::string MQMessageConst::PROPERTY_MQ2_FLAG = "MQ2_FLAG";
const std::string MQMessageConst::PROPERTY_RECONSUME_TIME = "RECONSUME_TIME";
const std::string MQMessageConst::PROPERTY_MSG_REGION = "MSG_REGION";
const std::string MQMessageConst::PROPERTY_TRACE_SWITCH = "TRACE_ON";
const std::string MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX = "UNIQ_KEY";
const std::string MQMessageConst::PROPERTY_MAX_RECONSUME_TIMES = "MAX_RECONSUME_TIMES";
const std::string MQMessageConst::PROPERTY_CONSUME_START_TIMESTAMP = "CONSUME_START_TIME";
const std::string MQMessageConst::PROPERTY_TRANSACTION_PREPARED_QUEUE_OFFSET = "TRAN_PREPARED_QUEUE_OFFSET";
const std::string MQMessageConst::PROPERTY_TRANSACTION_CHECK_TIMES = "TRANSACTION_CHECK_TIMES";
const std::string MQMessageConst::PROPERTY_CHECK_IMMUNITY_TIME_IN_SECONDS = "CHECK_IMMUNITY_TIME_IN_SECONDS";
const std::string MQMessageConst::PROPERTY_INSTANCE_ID = "INSTANCE_ID";

const std::string MQMessageConst::PROPERTY_ALREADY_COMPRESSED_FLAG = "__ALREADY_COMPRESSED__";

const std::string MQMessageConst::KEY_SEPARATOR = " ";

static const std::string EMPTY_STRING = "";

MQMessage::MQMessage() : MQMessage(null, "*", null, 0, null, true) {}

MQMessage::MQMessage(const std::string& topic, const std::string& body) : MQMessage(topic, "*", null, 0, body, true) {}

MQMessage::MQMessage(const std::string& topic, const std::string& tags, const std::string& body)
    : MQMessage(topic, tags, null, 0, body, true) {}

MQMessage::MQMessage(const std::string& topic,
                     const std::string& tags,
                     const std::string& keys,
                     const std::string& body)
    : MQMessage(topic, tags, keys, 0, body, true) {}

MQMessage::MQMessage(const std::string& topic,
                     const std::string& tags,
                     const std::string& keys,
                     const int flag,
                     const std::string& body,
                     bool waitStoreMsgOK)
    : m_topic(topic), m_flag(flag), m_body(body) {
  if (tags.length() > 0) {
    setTags(tags);
  }

  if (keys.length() > 0) {
    setKeys(keys);
  }

  setWaitStoreMsgOK(waitStoreMsgOK);
}

MQMessage::MQMessage(const MQMessage& other) {
  m_topic = other.m_topic;
  m_flag = other.m_flag;
  m_properties = other.m_properties;
  m_body = other.m_body;
}

MQMessage& MQMessage::operator=(const MQMessage& other) {
  if (this != &other) {
    m_topic = other.m_topic;
    m_flag = other.m_flag;
    m_properties = other.m_properties;
    m_body = other.m_body;
  }
  return *this;
}

MQMessage::~MQMessage() = default;

const std::string& MQMessage::getProperty(const std::string& name) const {
  const auto it = m_properties.find(name);
  if (it != m_properties.end()) {
    return it->second;
  }
  return EMPTY_STRING;
}

void MQMessage::putProperty(const std::string& name, const std::string& value) {
  m_properties[name] = value;
}

void MQMessage::clearProperty(const std::string& name) {
  m_properties.erase(name);
}

const std::string& MQMessage::getTopic() const {
  return m_topic;
}

void MQMessage::setTopic(const std::string& topic) {
  m_topic = topic;
}

void MQMessage::setTopic(const char* body, int len) {
  m_topic.clear();
  m_topic.append(body, len);
}

const std::string& MQMessage::getTags() const {
  return getProperty(MQMessageConst::PROPERTY_TAGS);
}

void MQMessage::setTags(const std::string& tags) {
  putProperty(MQMessageConst::PROPERTY_TAGS, tags);
}

const std::string& MQMessage::getKeys() const {
  return getProperty(MQMessageConst::PROPERTY_KEYS);
}

void MQMessage::setKeys(const std::string& keys) {
  putProperty(MQMessageConst::PROPERTY_KEYS, keys);
}

void MQMessage::setKeys(const std::vector<std::string>& keys) {
  if (keys.empty()) {
    return;
  }

  std::vector<std::string>::const_iterator it = keys.begin();
  std::string str;
  str += *it;
  it++;

  for (; it != keys.end(); it++) {
    str += MQMessageConst::KEY_SEPARATOR;
    str += *it;
  }

  setKeys(str);
}

int MQMessage::getDelayTimeLevel() const {
  std::string tmp = getProperty(MQMessageConst::PROPERTY_DELAY_TIME_LEVEL);
  if (!tmp.empty()) {
    return atoi(tmp.c_str());
  }
  return 0;
}

void MQMessage::setDelayTimeLevel(int level) {
  putProperty(MQMessageConst::PROPERTY_DELAY_TIME_LEVEL, UtilAll::to_string(level));
}

bool MQMessage::isWaitStoreMsgOK() const {
  std::string tmp = getProperty(MQMessageConst::PROPERTY_WAIT_STORE_MSG_OK);
  return tmp.empty() || UtilAll::stob(tmp);
}

void MQMessage::setWaitStoreMsgOK(bool waitStoreMsgOK) {
  putProperty(MQMessageConst::PROPERTY_WAIT_STORE_MSG_OK, UtilAll::to_string(waitStoreMsgOK));
}

int MQMessage::getFlag() const {
  return m_flag;
}

void MQMessage::setFlag(int flag) {
  m_flag = flag;
}

const std::string& MQMessage::getBody() const {
  // TODO: return MemoryBlockPtr2
  return m_body;
}

void MQMessage::setBody(const char* body, int len) {
  m_body.clear();
  m_body.append(body, len);
}

void MQMessage::setBody(const std::string& body) {
  m_body = body;
}

void MQMessage::setBody(std::string&& body) {
  m_body = std::forward<std::string>(body);
}

const std::string& MQMessage::getTransactionId() const {
  return m_transactionId;
}

void MQMessage::setTransactionId(const std::string& transactionId) {
  m_transactionId = transactionId;
}

const std::map<std::string, std::string>& MQMessage::getProperties() const {
  return m_properties;
}

void MQMessage::setProperties(const std::map<std::string, std::string>& properties) {
  m_properties = properties;
}

void MQMessage::setProperties(std::map<std::string, std::string>&& properties) {
  m_properties = std::forward<std::map<std::string, std::string>>(properties);
}

}  // namespace rocketmq
