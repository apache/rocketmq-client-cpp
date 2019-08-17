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

const std::string MQMessage::PROPERTY_KEYS = "KEYS";
const std::string MQMessage::PROPERTY_TAGS = "TAGS";
const std::string MQMessage::PROPERTY_WAIT_STORE_MSG_OK = "WAIT";
const std::string MQMessage::PROPERTY_DELAY_TIME_LEVEL = "DELAY";
const std::string MQMessage::PROPERTY_RETRY_TOPIC = "RETRY_TOPIC";
const std::string MQMessage::PROPERTY_REAL_TOPIC = "REAL_TOPIC";
const std::string MQMessage::PROPERTY_REAL_QUEUE_ID = "REAL_QID";
const std::string MQMessage::PROPERTY_TRANSACTION_PREPARED = "TRAN_MSG";
const std::string MQMessage::PROPERTY_PRODUCER_GROUP = "PGROUP";
const std::string MQMessage::PROPERTY_MIN_OFFSET = "MIN_OFFSET";
const std::string MQMessage::PROPERTY_MAX_OFFSET = "MAX_OFFSET";
const std::string MQMessage::PROPERTY_BUYER_ID = "BUYER_ID";
const std::string MQMessage::PROPERTY_ORIGIN_MESSAGE_ID = "ORIGIN_MESSAGE_ID";
const std::string MQMessage::PROPERTY_TRANSFER_FLAG = "TRANSFER_FLAG";
const std::string MQMessage::PROPERTY_CORRECTION_FLAG = "CORRECTION_FLAG";
const std::string MQMessage::PROPERTY_MQ2_FLAG = "MQ2_FLAG";
const std::string MQMessage::PROPERTY_RECONSUME_TIME = "RECONSUME_TIME";
const std::string MQMessage::PROPERTY_MSG_REGION = "MSG_REGION";
const std::string MQMessage::PROPERTY_TRACE_SWITCH = "TRACE_ON";
const std::string MQMessage::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX = "UNIQ_KEY";
const std::string MQMessage::PROPERTY_MAX_RECONSUME_TIMES = "MAX_RECONSUME_TIMES";
const std::string MQMessage::PROPERTY_CONSUME_START_TIMESTAMP = "CONSUME_START_TIME";
const std::string MQMessage::PROPERTY_TRANSACTION_PREPARED_QUEUE_OFFSET = "TRAN_PREPARED_QUEUE_OFFSET";
const std::string MQMessage::PROPERTY_TRANSACTION_CHECK_TIMES = "TRANSACTION_CHECK_TIMES";
const std::string MQMessage::PROPERTY_CHECK_IMMUNITY_TIME_IN_SECONDS = "CHECK_IMMUNITY_TIME_IN_SECONDS";

const std::string MQMessage::KEY_SEPARATOR = " ";

static const std::string EMPTY_STRING = "";

MQMessage::MQMessage() {
  Init("", "", "", 0, "", true);
}

MQMessage::MQMessage(const std::string& topic, const std::string& body) {
  Init(topic, "", "", 0, body, true);
}

MQMessage::MQMessage(const std::string& topic, const std::string& tags, const std::string& body) {
  Init(topic, tags, "", 0, body, true);
}

MQMessage::MQMessage(const std::string& topic,
                     const std::string& tags,
                     const std::string& keys,
                     const std::string& body) {
  Init(topic, tags, keys, 0, body, true);
}

MQMessage::MQMessage(const std::string& topic,
                     const std::string& tags,
                     const std::string& keys,
                     const int flag,
                     const std::string& body,
                     bool waitStoreMsgOK) {
  Init(topic, tags, keys, flag, body, waitStoreMsgOK);
}

MQMessage::~MQMessage() {
  m_properties.clear();
}

MQMessage::MQMessage(const MQMessage& other) {
  m_body = other.m_body;
  m_topic = other.m_topic;
  m_flag = other.m_flag;
  m_sysFlag = other.m_sysFlag;
  m_properties = other.m_properties;
}

MQMessage& MQMessage::operator=(const MQMessage& other) {
  if (this != &other) {
    m_body = other.m_body;
    m_topic = other.m_topic;
    m_flag = other.m_flag;
    m_sysFlag = other.m_sysFlag;
    m_properties = other.m_properties;
  }
  return *this;
}

void MQMessage::setProperty(const std::string& name, const std::string& value) {
  if (PROPERTY_TRANSACTION_PREPARED == name) {
    if (!value.empty() && value == "true") {
      m_sysFlag |= MessageSysFlag::TransactionPreparedType;
    } else {
      m_sysFlag &= ~MessageSysFlag::TransactionPreparedType;
    }
  }
  m_properties[name] = value;
}

void MQMessage::setPropertyInternal(const std::string& name, const std::string& value) {
  m_properties[name] = value;
}

const std::string& MQMessage::getProperty(const std::string& name) const {
  std::map<std::string, std::string>::const_iterator it = m_properties.find(name);
  if (it == m_properties.end()) {
    return EMPTY_STRING;
  } else {
    return it->second;
  }
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
  return getProperty(PROPERTY_TAGS);
}

void MQMessage::setTags(const std::string& tags) {
  setPropertyInternal(PROPERTY_TAGS, tags);
}

const std::string& MQMessage::getKeys() const {
  return getProperty(PROPERTY_KEYS);
}

void MQMessage::setKeys(const std::string& keys) {
  setPropertyInternal(PROPERTY_KEYS, keys);
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
    str += KEY_SEPARATOR;
    str += *it;
  }

  setKeys(str);
}

int MQMessage::getDelayTimeLevel() const {
  std::string tmp = getProperty(PROPERTY_DELAY_TIME_LEVEL);
  if (!tmp.empty()) {
    return atoi(tmp.c_str());
  }
  return 0;
}

void MQMessage::setDelayTimeLevel(int level) {
  char tmp[16];
  sprintf(tmp, "%d", level);

  setPropertyInternal(PROPERTY_DELAY_TIME_LEVEL, tmp);
}

bool MQMessage::isWaitStoreMsgOK() const {
  std::string tmp = getProperty(PROPERTY_WAIT_STORE_MSG_OK);
  if (tmp.empty()) {
    return true;
  } else {
    return (tmp == "true") ? true : false;
  }
}

void MQMessage::setWaitStoreMsgOK(bool waitStoreMsgOK) {
  if (waitStoreMsgOK) {
    setPropertyInternal(PROPERTY_WAIT_STORE_MSG_OK, "true");
  } else {
    setPropertyInternal(PROPERTY_WAIT_STORE_MSG_OK, "false");
  }
}

int MQMessage::getFlag() const {
  return m_flag;
}

void MQMessage::setFlag(int flag) {
  m_flag = flag;
}

int MQMessage::getSysFlag() const {
  return m_sysFlag;
}

void MQMessage::setSysFlag(int sysFlag) {
  m_sysFlag = sysFlag;
}

const std::string& MQMessage::getBody() const {
  return m_body;
}

void MQMessage::setBody(const char* body, int len) {
  m_body.clear();
  m_body.append(body, len);
}

void MQMessage::setBody(const std::string& body) {
  m_body.clear();
  m_body.append(body);
}

std::map<std::string, std::string> MQMessage::getProperties() const {
  return m_properties;
}

void MQMessage::setProperties(std::map<std::string, std::string>& properties) {
  m_properties = properties;

  std::map<std::string, std::string>::const_iterator it = m_properties.find(PROPERTY_TRANSACTION_PREPARED);
  if (it != m_properties.end()) {
    std::string tranMsg = it->second;
    if (!tranMsg.empty() && tranMsg == "true") {
      m_sysFlag |= MessageSysFlag::TransactionPreparedType;
    } else {
      m_sysFlag &= ~MessageSysFlag::TransactionPreparedType;
    }
  }
}

void MQMessage::setPropertiesInternal(std::map<std::string, std::string>& properties) {
  m_properties = properties;
}

void MQMessage::Init(const std::string& topic,
                     const std::string& tags,
                     const std::string& keys,
                     const int flag,
                     const std::string& body,
                     bool waitStoreMsgOK) {
  m_topic = topic;
  m_flag = flag;
  m_sysFlag = 0;
  m_body = body;

  if (tags.length() > 0) {
    setTags(tags);
  }

  if (keys.length() > 0) {
    setKeys(keys);
  }

  setWaitStoreMsgOK(waitStoreMsgOK);
}

}  // namespace rocketmq
