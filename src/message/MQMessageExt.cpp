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

MQMessageExt::MQMessageExt() : MQMessageExt(0, 0, nullptr, 0, nullptr, null) {}

MQMessageExt::MQMessageExt(int queueId,
                           int64_t bornTimestamp,
                           const struct sockaddr* bornHost,
                           int64_t storeTimestamp,
                           const struct sockaddr* storeHost,
                           const std::string& msgId)
    : MQMessage(std::make_shared<MessageExtImpl>(queueId, bornTimestamp, bornHost, storeTimestamp, storeHost, msgId)) {}

MQMessageExt::~MQMessageExt() = default;

int32_t MQMessageExt::getStoreSize() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getStoreSize();
}

void MQMessageExt::setStoreSize(int32_t storeSize) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setStoreSize(storeSize);
}

int32_t MQMessageExt::getBodyCRC() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getBodyCRC();
}

void MQMessageExt::setBodyCRC(int32_t bodyCRC) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setBodyCRC(bodyCRC);
}

int32_t MQMessageExt::getQueueId() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getQueueId();
}

void MQMessageExt::setQueueId(int32_t queueId) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setQueueId(queueId);
}

int64_t MQMessageExt::getQueueOffset() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getQueueOffset();
}

void MQMessageExt::setQueueOffset(int64_t queueOffset) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setQueueOffset(queueOffset);
}

int64_t MQMessageExt::getCommitLogOffset() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getCommitLogOffset();
}

void MQMessageExt::setCommitLogOffset(int64_t physicOffset) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setCommitLogOffset(physicOffset);
}

int32_t MQMessageExt::getSysFlag() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getSysFlag();
}

void MQMessageExt::setSysFlag(int32_t sysFlag) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setSysFlag(sysFlag);
}

int64_t MQMessageExt::getBornTimestamp() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getBornTimestamp();
}

void MQMessageExt::setBornTimestamp(int64_t bornTimestamp) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setBornTimestamp(bornTimestamp);
}

std::string MQMessageExt::getBornHostString() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getBornHostString();
}

const struct sockaddr* MQMessageExt::getBornHost() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getBornHost();
}

void MQMessageExt::setBornHost(const struct sockaddr* bornHost) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setBornHost(bornHost);
}

int64_t MQMessageExt::getStoreTimestamp() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getStoreTimestamp();
}

void MQMessageExt::setStoreTimestamp(int64_t storeTimestamp) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setStoreTimestamp(storeTimestamp);
}

std::string MQMessageExt::getStoreHostString() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getStoreHostString();
}

const struct sockaddr* MQMessageExt::getStoreHost() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getStoreHost();
}

void MQMessageExt::setStoreHost(const struct sockaddr* storeHost) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setStoreHost(storeHost);
}

int32_t MQMessageExt::getReconsumeTimes() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getReconsumeTimes();
}

void MQMessageExt::setReconsumeTimes(int32_t reconsumeTimes) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setReconsumeTimes(reconsumeTimes);
}

int64_t MQMessageExt::getPreparedTransactionOffset() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getPreparedTransactionOffset();
}

void MQMessageExt::setPreparedTransactionOffset(int64_t preparedTransactionOffset) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setPreparedTransactionOffset(preparedTransactionOffset);
}

const std::string& MQMessageExt::getMsgId() const {
  return std::dynamic_pointer_cast<MessageExt>(message_impl_)->getMsgId();
}

void MQMessageExt::setMsgId(const std::string& msgId) {
  std::dynamic_pointer_cast<MessageExt>(message_impl_)->setMsgId(msgId);
}

}  // namespace rocketmq
