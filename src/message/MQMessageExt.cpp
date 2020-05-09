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
#include "MQMessageExt.h"

#include <cstring>

#include "MessageClientIDSetter.h"
#include "MessageSysFlag.h"
#include "SocketUtil.h"
#include "TopicFilterType.h"

namespace rocketmq {

MQMessageExt::MQMessageExt() : MQMessageExt(0, 0, nullptr, 0, nullptr, "") {}

MQMessageExt::MQMessageExt(int queueId,
                           int64_t bornTimestamp,
                           const struct sockaddr* bornHost,
                           int64_t storeTimestamp,
                           const struct sockaddr* storeHost,
                           std::string msgId)
    : m_storeSize(0),
      m_bodyCRC(0),
      m_queueId(queueId),
      m_queueOffset(0),
      m_commitLogOffset(0),
      m_sysFlag(0),
      m_bornTimestamp(bornTimestamp),
      m_bornHost(nullptr),
      m_storeTimestamp(storeTimestamp),
      m_storeHost(nullptr),
      m_reconsumeTimes(3),
      m_preparedTransactionOffset(0),
      m_msgId(msgId) {
  m_bornHost = copySocketAddress(m_bornHost, bornHost);
  m_storeHost = copySocketAddress(m_storeHost, storeHost);
}

MQMessageExt::~MQMessageExt() {
  free(m_bornHost);
  free(m_storeHost);
}

TopicFilterType MQMessageExt::parseTopicFilterType(int32_t sysFlag) {
  if ((sysFlag & MessageSysFlag::MultiTagsFlag) == MessageSysFlag::MultiTagsFlag) {
    return MULTI_TAG;
  }
  return SINGLE_TAG;
}

int32_t MQMessageExt::getStoreSize() const {
  return m_storeSize;
}

void MQMessageExt::setStoreSize(int32_t storeSize) {
  m_storeSize = storeSize;
}

int32_t MQMessageExt::getBodyCRC() const {
  return m_bodyCRC;
}

void MQMessageExt::setBodyCRC(int32_t bodyCRC) {
  m_bodyCRC = bodyCRC;
}

int32_t MQMessageExt::getQueueId() const {
  return m_queueId;
}

void MQMessageExt::setQueueId(int32_t queueId) {
  m_queueId = queueId;
}

int64_t MQMessageExt::getQueueOffset() const {
  return m_queueOffset;
}

void MQMessageExt::setQueueOffset(int64_t queueOffset) {
  m_queueOffset = queueOffset;
}

int64_t MQMessageExt::getCommitLogOffset() const {
  return m_commitLogOffset;
}

void MQMessageExt::setCommitLogOffset(int64_t physicOffset) {
  m_commitLogOffset = physicOffset;
}

int32_t MQMessageExt::getSysFlag() const {
  return m_sysFlag;
}

void MQMessageExt::setSysFlag(int32_t sysFlag) {
  m_sysFlag = sysFlag;
}

int64_t MQMessageExt::getBornTimestamp() const {
  return m_bornTimestamp;
}

void MQMessageExt::setBornTimestamp(int64_t bornTimestamp) {
  m_bornTimestamp = bornTimestamp;
}

const struct sockaddr* MQMessageExt::getBornHost() const {
  return m_bornHost;
}

std::string MQMessageExt::getBornHostString() const {
  return socketAddress2String(m_bornHost);
}

void MQMessageExt::setBornHost(const struct sockaddr* bornHost) {
  m_bornHost = copySocketAddress(m_bornHost, bornHost);
}

int64_t MQMessageExt::getStoreTimestamp() const {
  return m_storeTimestamp;
}

void MQMessageExt::setStoreTimestamp(int64_t storeTimestamp) {
  m_storeTimestamp = storeTimestamp;
}

const struct sockaddr* MQMessageExt::getStoreHost() const {
  return m_storeHost;
}

std::string MQMessageExt::getStoreHostString() const {
  return socketAddress2String(m_storeHost);
}

void MQMessageExt::setStoreHost(const struct sockaddr* storeHost) {
  m_storeHost = copySocketAddress(m_storeHost, storeHost);
}

const std::string& MQMessageExt::getMsgId() const {
  return m_msgId;
}

void MQMessageExt::setMsgId(const std::string& msgId) {
  m_msgId = msgId;
}

int32_t MQMessageExt::getReconsumeTimes() const {
  return m_reconsumeTimes;
}

void MQMessageExt::setReconsumeTimes(int32_t reconsumeTimes) {
  m_reconsumeTimes = reconsumeTimes;
}

int64_t MQMessageExt::getPreparedTransactionOffset() const {
  return m_preparedTransactionOffset;
}

void MQMessageExt::setPreparedTransactionOffset(int64_t preparedTransactionOffset) {
  m_preparedTransactionOffset = preparedTransactionOffset;
}

const std::string& MQMessageClientExt::getOffsetMsgId() const {
  return MQMessageExt::getMsgId();
}

void MQMessageClientExt::setOffsetMsgId(const std::string& offsetMsgId) {
  return MQMessageExt::setMsgId(offsetMsgId);
}

const std::string& MQMessageClientExt::getMsgId() const {
  const auto& uniqID = MessageClientIDSetter::getUniqID(*this);
  if (uniqID.empty()) {
    return getOffsetMsgId();
  } else {
    return uniqID;
  }
}

void MQMessageClientExt::setMsgId(const std::string& msgId) {
  // DO NOTHING
  // MessageClientIDSetter::setUniqID(*this);
}

}  // namespace rocketmq
