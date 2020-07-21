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
  return dynamic_cast<MessageExt*>(message_impl_.get())->getStoreSize();
}

void MQMessageExt::setStoreSize(int32_t storeSize) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setStoreSize(storeSize);
}

int32_t MQMessageExt::getBodyCRC() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getBodyCRC();
}

void MQMessageExt::setBodyCRC(int32_t bodyCRC) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setBodyCRC(bodyCRC);
}

int32_t MQMessageExt::getQueueId() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getQueueId();
}

void MQMessageExt::setQueueId(int32_t queueId) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setQueueId(queueId);
}

int64_t MQMessageExt::getQueueOffset() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getQueueOffset();
}

void MQMessageExt::setQueueOffset(int64_t queueOffset) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setQueueOffset(queueOffset);
}

int64_t MQMessageExt::getCommitLogOffset() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getCommitLogOffset();
}

void MQMessageExt::setCommitLogOffset(int64_t physicOffset) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setCommitLogOffset(physicOffset);
}

int32_t MQMessageExt::getSysFlag() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getSysFlag();
}

void MQMessageExt::setSysFlag(int32_t sysFlag) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setSysFlag(sysFlag);
}

int64_t MQMessageExt::getBornTimestamp() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getBornTimestamp();
}

void MQMessageExt::setBornTimestamp(int64_t bornTimestamp) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setBornTimestamp(bornTimestamp);
}

std::string MQMessageExt::getBornHostString() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getBornHostString();
}

const struct sockaddr* MQMessageExt::getBornHost() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getBornHost();
}

void MQMessageExt::setBornHost(const struct sockaddr* bornHost) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setBornHost(bornHost);
}

int64_t MQMessageExt::getStoreTimestamp() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getStoreTimestamp();
}

void MQMessageExt::setStoreTimestamp(int64_t storeTimestamp) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setStoreTimestamp(storeTimestamp);
}

std::string MQMessageExt::getStoreHostString() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getStoreHostString();
}

const struct sockaddr* MQMessageExt::getStoreHost() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getStoreHost();
}

void MQMessageExt::setStoreHost(const struct sockaddr* storeHost) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setStoreHost(storeHost);
}

int32_t MQMessageExt::getReconsumeTimes() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getReconsumeTimes();
}

void MQMessageExt::setReconsumeTimes(int32_t reconsumeTimes) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setReconsumeTimes(reconsumeTimes);
}

int64_t MQMessageExt::getPreparedTransactionOffset() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getPreparedTransactionOffset();
}

void MQMessageExt::setPreparedTransactionOffset(int64_t preparedTransactionOffset) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setPreparedTransactionOffset(preparedTransactionOffset);
}

const std::string& MQMessageExt::getMsgId() const {
  return dynamic_cast<MessageExt*>(message_impl_.get())->getMsgId();
}

void MQMessageExt::setMsgId(const std::string& msgId) {
  dynamic_cast<MessageExt*>(message_impl_.get())->setMsgId(msgId);
}

}  // namespace rocketmq
