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
#include "MessageExtImpl.h"

#include <sstream>  // std::stringstream

#include "MessageClientIDSetter.h"
#include "MessageSysFlag.h"
#include "SocketUtil.h"
#include "UtilAll.h"

namespace rocketmq {

// ============================
// MessageExtImpl
// ============================

MessageExtImpl::MessageExtImpl() : MessageExtImpl(0, 0, nullptr, 0, nullptr, null) {}

MessageExtImpl::MessageExtImpl(int queueId,
                               int64_t bornTimestamp,
                               const struct sockaddr* bornHost,
                               int64_t storeTimestamp,
                               const struct sockaddr* storeHost,
                               const std::string& msgId)
    : store_size_(0),
      body_crc_(0),
      queue_id_(queueId),
      queue_offset_(0),
      commit_log_offset_(0),
      sys_flag_(0),
      born_timestamp_(bornTimestamp),
      born_host_(nullptr),
      store_timestamp_(storeTimestamp),
      store_host_(nullptr),
      reconsume_times_(3),
      prepared_transaction_offset_(0),
      msg_id_(msgId) {
  born_host_ = copySocketAddress(born_host_, bornHost);
  store_host_ = copySocketAddress(store_host_, storeHost);
}

MessageExtImpl::~MessageExtImpl() {
  free(born_host_);
  free(store_host_);
}

TopicFilterType MessageExtImpl::parseTopicFilterType(int32_t sysFlag) {
  if ((sysFlag & MessageSysFlag::MultiTagsFlag) == MessageSysFlag::MultiTagsFlag) {
    return MULTI_TAG;
  }
  return SINGLE_TAG;
}

int32_t MessageExtImpl::getStoreSize() const {
  return store_size_;
}

void MessageExtImpl::setStoreSize(int32_t storeSize) {
  store_size_ = storeSize;
}

int32_t MessageExtImpl::getBodyCRC() const {
  return body_crc_;
}

void MessageExtImpl::setBodyCRC(int32_t bodyCRC) {
  body_crc_ = bodyCRC;
}

int32_t MessageExtImpl::getQueueId() const {
  return queue_id_;
}

void MessageExtImpl::setQueueId(int32_t queueId) {
  queue_id_ = queueId;
}

int64_t MessageExtImpl::getQueueOffset() const {
  return queue_offset_;
}

void MessageExtImpl::setQueueOffset(int64_t queueOffset) {
  queue_offset_ = queueOffset;
}

int64_t MessageExtImpl::getCommitLogOffset() const {
  return commit_log_offset_;
}

void MessageExtImpl::setCommitLogOffset(int64_t physicOffset) {
  commit_log_offset_ = physicOffset;
}

int32_t MessageExtImpl::getSysFlag() const {
  return sys_flag_;
}

void MessageExtImpl::setSysFlag(int32_t sysFlag) {
  sys_flag_ = sysFlag;
}

int64_t MessageExtImpl::getBornTimestamp() const {
  return born_timestamp_;
}

void MessageExtImpl::setBornTimestamp(int64_t bornTimestamp) {
  born_timestamp_ = bornTimestamp;
}

const struct sockaddr* MessageExtImpl::getBornHost() const {
  return born_host_;
}

std::string MessageExtImpl::getBornHostString() const {
  return socketAddress2String(born_host_);
}

void MessageExtImpl::setBornHost(const struct sockaddr* bornHost) {
  born_host_ = copySocketAddress(born_host_, bornHost);
}

int64_t MessageExtImpl::getStoreTimestamp() const {
  return store_timestamp_;
}

void MessageExtImpl::setStoreTimestamp(int64_t storeTimestamp) {
  store_timestamp_ = storeTimestamp;
}

const struct sockaddr* MessageExtImpl::getStoreHost() const {
  return store_host_;
}

std::string MessageExtImpl::getStoreHostString() const {
  return socketAddress2String(store_host_);
}

void MessageExtImpl::setStoreHost(const struct sockaddr* storeHost) {
  store_host_ = copySocketAddress(store_host_, storeHost);
}

const std::string& MessageExtImpl::getMsgId() const {
  return msg_id_;
}

void MessageExtImpl::setMsgId(const std::string& msgId) {
  msg_id_ = msgId;
}

int32_t MessageExtImpl::getReconsumeTimes() const {
  return reconsume_times_;
}

void MessageExtImpl::setReconsumeTimes(int32_t reconsumeTimes) {
  reconsume_times_ = reconsumeTimes;
}

int64_t MessageExtImpl::getPreparedTransactionOffset() const {
  return prepared_transaction_offset_;
}

void MessageExtImpl::setPreparedTransactionOffset(int64_t preparedTransactionOffset) {
  prepared_transaction_offset_ = preparedTransactionOffset;
}

std::string MessageExtImpl::toString() const {
  std::stringstream ss;
  ss << "MessageExt [queueId=" << queue_id_ << ", storeSize=" << store_size_ << ", queueOffset=" << queue_offset_
     << ", sysFlag=" << sys_flag_ << ", bornTimestamp=" << born_timestamp_ << ", bornHost=" << getBornHostString()
     << ", storeTimestamp=" << store_timestamp_ << ", storeHost=" << getStoreHostString() << ", msgId=" << getMsgId()
     << ", commitLogOffset=" << commit_log_offset_ << ", bodyCRC=" << body_crc_
     << ", reconsumeTimes=" << reconsume_times_ << ", preparedTransactionOffset=" << prepared_transaction_offset_
     << ", toString()=" << MessageImpl::toString() << "]";
  return ss.str();
}

// ============================
// MessageClientExtImpl
// ============================

const std::string& MessageClientExtImpl::getMsgId() const {
  const auto& unique_id = MessageClientIDSetter::getUniqID(*this);
  return unique_id.empty() ? getOffsetMsgId() : unique_id;
}

void MessageClientExtImpl::setMsgId(const std::string& msgId) {
  // DO NOTHING
  // MessageClientIDSetter::setUniqID(*this);
}

const std::string& MessageClientExtImpl::getOffsetMsgId() const {
  return MessageExtImpl::getMsgId();
}

void MessageClientExtImpl::setOffsetMsgId(const std::string& offsetMsgId) {
  return MessageExtImpl::setMsgId(offsetMsgId);
}

}  // namespace rocketmq
