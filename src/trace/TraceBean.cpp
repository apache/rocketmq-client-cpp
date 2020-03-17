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

#include "TraceBean.h"
#include <string>
#include <vector>

namespace rocketmq {
TraceBean::TraceBean()
    : m_topic("null"),
      m_msgId("null"),
      m_offsetMsgId("null"),
      m_tags("null"),
      m_keys("null"),
      m_storeHost("null"),
      m_clientHost("null") {}

TraceBean::~TraceBean() {}

const std::string& TraceBean::getTopic() const {
  return m_topic;
}

void TraceBean::setTopic(const std::string& topic) {
  m_topic = topic;
}

const std::string& TraceBean::getMsgId() const {
  return m_msgId;
}

void TraceBean::setMsgId(const std::string& msgId) {
  m_msgId = msgId;
}

const std::string& TraceBean::getOffsetMsgId() const {
  return m_offsetMsgId;
}

void TraceBean::setOffsetMsgId(const std::string& offsetMsgId) {
  m_offsetMsgId = offsetMsgId;
}

const std::string& TraceBean::getTags() const {
  return m_tags;
}

void TraceBean::setTags(const std::string& tags) {
  m_tags = tags;
}

const std::string& TraceBean::getKeys() const {
  return m_keys;
}

void TraceBean::setKeys(const std::string& keys) {
  m_keys = keys;
}

const std::string& TraceBean::getStoreHost() const {
  return m_storeHost;
}

void TraceBean::setStoreHost(const std::string& storeHost) {
  m_storeHost = storeHost;
}

const std::string& TraceBean::getClientHost() const {
  return m_clientHost;
}

void TraceBean::setClientHost(const std::string& clientHost) {
  m_clientHost = clientHost;
}

TraceMessageType TraceBean::getMsgType() const {
  return m_msgType;
}

void TraceBean::setMsgType(TraceMessageType msgType) {
  m_msgType = msgType;
}

long long int TraceBean::getStoreTime() const {
  return m_storeTime;
}

void TraceBean::setStoreTime(long long int storeTime) {
  m_storeTime = storeTime;
}

int TraceBean::getRetryTimes() const {
  return m_retryTimes;
}

void TraceBean::setRetryTimes(int retryTimes) {
  m_retryTimes = retryTimes;
}

int TraceBean::getBodyLength() const {
  return m_bodyLength;
}

void TraceBean::setBodyLength(int bodyLength) {
  m_bodyLength = bodyLength;
}
}  // namespace rocketmq