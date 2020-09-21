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

#include "MessageExtImpl.h"
#include "UtilAll.h"  // null

namespace rocketmq {

std::vector<MQMessageExt> MQMessageExt::from_list(std::vector<MessageExtPtr>& msg_list) {
  std::vector<MQMessageExt> message_list;
  message_list.reserve(msg_list.size());
  for (const auto& msg : msg_list) {
    message_list.emplace_back(msg);
  }
  return message_list;
}

MQMessageExt::MQMessageExt() : MQMessageExt(0, 0, nullptr, 0, nullptr, null) {}

MQMessageExt::MQMessageExt(int queueId,
                           int64_t bornTimestamp,
                           const struct sockaddr* bornHost,
                           int64_t storeTimestamp,
                           const struct sockaddr* storeHost,
                           const std::string& msgId)
    : MQMessage(std::make_shared<MessageExtImpl>(queueId, bornTimestamp, bornHost, storeTimestamp, storeHost, msgId)) {}

MQMessageExt::~MQMessageExt() = default;

int32_t MQMessageExt::store_size() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->store_size();
}

void MQMessageExt::set_store_size(int32_t storeSize) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_store_size(storeSize);
}

int32_t MQMessageExt::body_crc() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->body_crc();
}

void MQMessageExt::set_body_crc(int32_t bodyCRC) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_body_crc(bodyCRC);
}

int32_t MQMessageExt::queue_id() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->queue_id();
}

void MQMessageExt::set_queue_id(int32_t queueId) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_queue_id(queueId);
}

int64_t MQMessageExt::queue_offset() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->queue_offset();
}

void MQMessageExt::set_queue_offset(int64_t queueOffset) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_queue_offset(queueOffset);
}

int64_t MQMessageExt::commit_log_offset() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->commit_log_offset();
}

void MQMessageExt::set_commit_log_offset(int64_t physicOffset) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_commit_log_offset(physicOffset);
}

int32_t MQMessageExt::sys_flag() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->sys_flag();
}

void MQMessageExt::set_sys_flag(int32_t sysFlag) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_sys_flag(sysFlag);
}

int64_t MQMessageExt::born_timestamp() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->born_timestamp();
}

void MQMessageExt::set_born_timestamp(int64_t bornTimestamp) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_born_timestamp(bornTimestamp);
}

std::string MQMessageExt::born_host_string() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->born_host_string();
}

const struct sockaddr* MQMessageExt::born_host() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->born_host();
}

void MQMessageExt::set_born_host(const struct sockaddr* bornHost) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_born_host(bornHost);
}

int64_t MQMessageExt::store_timestamp() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->store_timestamp();
}

void MQMessageExt::set_store_timestamp(int64_t storeTimestamp) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_store_timestamp(storeTimestamp);
}

std::string MQMessageExt::store_host_string() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->store_host_string();
}

const struct sockaddr* MQMessageExt::store_host() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->store_host();
}

void MQMessageExt::set_store_host(const struct sockaddr* storeHost) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_store_host(storeHost);
}

int32_t MQMessageExt::reconsume_times() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->reconsume_times();
}

void MQMessageExt::set_reconsume_times(int32_t reconsumeTimes) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_reconsume_times(reconsumeTimes);
}

int64_t MQMessageExt::prepared_transaction_offset() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->prepared_transaction_offset();
}

void MQMessageExt::set_prepared_transaction_offset(int64_t preparedTransactionOffset) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_prepared_transaction_offset(preparedTransactionOffset);
}

const std::string& MQMessageExt::msg_id() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->msg_id();
}

void MQMessageExt::set_msg_id(const std::string& msgId) {
  dynamic_cast<MessageExt*>(message_impl_.get())->set_msg_id(msgId);
}

}  // namespace rocketmq
