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
      born_host_(SockaddrToStorage(bornHost)),
      store_timestamp_(storeTimestamp),
      store_host_(SockaddrToStorage(storeHost)),
      reconsume_times_(3),
      prepared_transaction_offset_(0),
      msg_id_(msgId) {}

MessageExtImpl::~MessageExtImpl() = default;

TopicFilterType MessageExtImpl::parseTopicFilterType(int32_t sysFlag) {
  if ((sysFlag & MessageSysFlag::MULTI_TAGS_FLAG) == MessageSysFlag::MULTI_TAGS_FLAG) {
    return MULTI_TAG;
  }
  return SINGLE_TAG;
}

int32_t MessageExtImpl::store_size() const {
  return store_size_;
}

void MessageExtImpl::set_store_size(int32_t storeSize) {
  store_size_ = storeSize;
}

int32_t MessageExtImpl::body_crc() const {
  return body_crc_;
}

void MessageExtImpl::set_body_crc(int32_t bodyCRC) {
  body_crc_ = bodyCRC;
}

int32_t MessageExtImpl::queue_id() const {
  return queue_id_;
}

void MessageExtImpl::set_queue_id(int32_t queueId) {
  queue_id_ = queueId;
}

int64_t MessageExtImpl::queue_offset() const {
  return queue_offset_;
}

void MessageExtImpl::set_queue_offset(int64_t queueOffset) {
  queue_offset_ = queueOffset;
}

int64_t MessageExtImpl::commit_log_offset() const {
  return commit_log_offset_;
}

void MessageExtImpl::set_commit_log_offset(int64_t physicOffset) {
  commit_log_offset_ = physicOffset;
}

int32_t MessageExtImpl::sys_flag() const {
  return sys_flag_;
}

void MessageExtImpl::set_sys_flag(int32_t sysFlag) {
  sys_flag_ = sysFlag;
}

int64_t MessageExtImpl::born_timestamp() const {
  return born_timestamp_;
}

void MessageExtImpl::set_born_timestamp(int64_t bornTimestamp) {
  born_timestamp_ = bornTimestamp;
}

const struct sockaddr* MessageExtImpl::born_host() const {
  return reinterpret_cast<sockaddr*>(born_host_.get());
}

std::string MessageExtImpl::born_host_string() const {
  return SockaddrToString(born_host());
}

void MessageExtImpl::set_born_host(const struct sockaddr* bornHost) {
  born_host_ = SockaddrToStorage(bornHost);
}

int64_t MessageExtImpl::store_timestamp() const {
  return store_timestamp_;
}

void MessageExtImpl::set_store_timestamp(int64_t storeTimestamp) {
  store_timestamp_ = storeTimestamp;
}

const struct sockaddr* MessageExtImpl::store_host() const {
  return reinterpret_cast<sockaddr*>(store_host_.get());
}

std::string MessageExtImpl::store_host_string() const {
  return SockaddrToString(store_host());
}

void MessageExtImpl::set_store_host(const struct sockaddr* storeHost) {
  store_host_ = SockaddrToStorage(storeHost);
}

const std::string& MessageExtImpl::msg_id() const {
  return msg_id_;
}

void MessageExtImpl::set_msg_id(const std::string& msgId) {
  msg_id_ = msgId;
}

int32_t MessageExtImpl::reconsume_times() const {
  return reconsume_times_;
}

void MessageExtImpl::set_reconsume_times(int32_t reconsumeTimes) {
  reconsume_times_ = reconsumeTimes;
}

int64_t MessageExtImpl::prepared_transaction_offset() const {
  return prepared_transaction_offset_;
}

void MessageExtImpl::set_prepared_transaction_offset(int64_t preparedTransactionOffset) {
  prepared_transaction_offset_ = preparedTransactionOffset;
}

std::string MessageExtImpl::toString() const {
  std::stringstream ss;
  ss << "MessageExt [queueId=" << queue_id_ << ", storeSize=" << store_size_ << ", queueOffset=" << queue_offset_
     << ", sysFlag=" << sys_flag_ << ", bornTimestamp=" << born_timestamp_ << ", bornHost=" << born_host_string()
     << ", storeTimestamp=" << store_timestamp_ << ", storeHost=" << store_host_string() << ", msgId=" << msg_id()
     << ", commitLogOffset=" << commit_log_offset_ << ", bodyCRC=" << body_crc_
     << ", reconsumeTimes=" << reconsume_times_ << ", preparedTransactionOffset=" << prepared_transaction_offset_
     << ", toString()=" << MessageImpl::toString() << "]";
  return ss.str();
}

// ============================
// MessageClientExtImpl
// ============================

const std::string& MessageClientExtImpl::msg_id() const {
  const auto& unique_id = MessageClientIDSetter::getUniqID(*this);
  return unique_id.empty() ? offset_msg_id() : unique_id;
}

void MessageClientExtImpl::set_msg_id(const std::string& msgId) {
  // DO NOTHING
  // MessageClientIDSetter::setUniqID(*this);
}

const std::string& MessageClientExtImpl::offset_msg_id() const {
  return MessageExtImpl::msg_id();
}

void MessageClientExtImpl::set_offset_msg_id(const std::string& offsetMsgId) {
  return MessageExtImpl::set_msg_id(offsetMsgId);
}

}  // namespace rocketmq
