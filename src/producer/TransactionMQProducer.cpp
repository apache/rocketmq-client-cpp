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

#include "TransactionMQProducer.h"
#include <MQVersion.h>

#include "TransactionMQProducerImpl.h"

namespace rocketmq {

TransactionMQProducer::TransactionMQProducer(const std::string& groupName) {
  impl = new TransactionMQProducerImpl(groupName);
}

TransactionMQProducer::~TransactionMQProducer() {
  delete impl;
}
void TransactionMQProducer::start() {
  impl->start();
}

void TransactionMQProducer::shutdown() {
  impl->shutdown();
}
std::string TransactionMQProducer::version() {
  std::string versions = impl->getClientVersionString();
  /*
  versions.append(", PROTOCOL VERSION: ")
      .append(MQVersion::GetVersionDesc(MQVersion::s_CurrentVersion))
      .append(", LANGUAGE: ")
      .append(MQVersion::s_CurrentLanguage);
  */
  return versions;
}
// start mqclient set
const std::string& TransactionMQProducer::getNamesrvAddr() const {
  return impl->getNamesrvAddr();
}

void TransactionMQProducer::setNamesrvAddr(const std::string& namesrvAddr) {
  impl->setNamesrvAddr(namesrvAddr);
}

const std::string& TransactionMQProducer::getNamesrvDomain() const {
  return impl->getNamesrvDomain();
}

void TransactionMQProducer::setNamesrvDomain(const std::string& namesrvDomain) {
  impl->setNamesrvDomain(namesrvDomain);
}
void TransactionMQProducer::setSessionCredentials(const std::string& accessKey,
                                                  const std::string& secretKey,
                                                  const std::string& accessChannel) {
  impl->setSessionCredentials(accessKey, secretKey, accessChannel);
}

const SessionCredentials& TransactionMQProducer::getSessionCredentials() const {
  return impl->getSessionCredentials();
}
const std::string& TransactionMQProducer::getInstanceName() const {
  return impl->getInstanceName();
}

void TransactionMQProducer::setInstanceName(const std::string& instanceName) {
  impl->setInstanceName(instanceName);
}

const std::string& TransactionMQProducer::getNameSpace() const {
  return impl->getNameSpace();
}

void TransactionMQProducer::setNameSpace(const std::string& nameSpace) {
  impl->setNameSpace(nameSpace);
}
const std::string& TransactionMQProducer::getGroupName() const {
  return impl->getGroupName();
}

void TransactionMQProducer::setGroupName(const std::string& groupName) {
  impl->setGroupName(groupName);
}
void TransactionMQProducer::setSendMsgTimeout(int sendMsgTimeout) {
  impl->setSendMsgTimeout(sendMsgTimeout);
}

int TransactionMQProducer::getSendMsgTimeout() const {
  return impl->getSendMsgTimeout();
}
void TransactionMQProducer::setTcpTransportPullThreadNum(int num) {
  impl->setTcpTransportPullThreadNum(num);
}
int TransactionMQProducer::getTcpTransportPullThreadNum() const {
  return impl->getTcpTransportPullThreadNum();
}

void TransactionMQProducer::setTcpTransportConnectTimeout(uint64_t timeout) {
  impl->setTcpTransportConnectTimeout(timeout);
}
uint64_t TransactionMQProducer::getTcpTransportConnectTimeout() const {
  return impl->getTcpTransportConnectTimeout();
}
void TransactionMQProducer::setTcpTransportTryLockTimeout(uint64_t timeout) {
  impl->setTcpTransportTryLockTimeout(timeout);
}
uint64_t TransactionMQProducer::getTcpTransportTryLockTimeout() const {
  return impl->getTcpTransportTryLockTimeout();
}
int TransactionMQProducer::getCompressMsgBodyOverHowmuch() const {
  return impl->getCompressMsgBodyOverHowmuch();
}

void TransactionMQProducer::setCompressMsgBodyOverHowmuch(int compressMsgBodyThreshold) {
  impl->setCompressMsgBodyOverHowmuch(compressMsgBodyThreshold);
}

int TransactionMQProducer::getCompressLevel() const {
  return impl->getCompressLevel();
}

void TransactionMQProducer::setCompressLevel(int compressLevel) {
  impl->setCompressLevel(compressLevel);
}

void TransactionMQProducer::setMaxMessageSize(int maxMessageSize) {
  impl->setMaxMessageSize(maxMessageSize);
}

int TransactionMQProducer::getMaxMessageSize() const {
  return impl->getMaxMessageSize();
}

void TransactionMQProducer::setLogLevel(elogLevel inputLevel) {
  impl->setLogLevel(inputLevel);
}

elogLevel TransactionMQProducer::getLogLevel() {
  return impl->getLogLevel();
}
void TransactionMQProducer::setLogFileSizeAndNum(int fileNum, long perFileSize) {
  impl->setLogFileSizeAndNum(fileNum, perFileSize);
}

void TransactionMQProducer::setUnitName(std::string unitName) {
  impl->setUnitName(unitName);
}
const std::string& TransactionMQProducer::getUnitName() const {
  return impl->getUnitName();
}
void TransactionMQProducer::setMessageTrace(bool messageTrace) {
  impl->setMessageTrace(messageTrace);
}
bool TransactionMQProducer::getMessageTrace() const {
  return impl->getMessageTrace();
}
std::shared_ptr<TransactionListener> TransactionMQProducer::getTransactionListener() {
  return impl->getTransactionListener();
}
void TransactionMQProducer::setTransactionListener(TransactionListener* listener) {
  impl->setTransactionListener(listener);
}
TransactionSendResult TransactionMQProducer::sendMessageInTransaction(MQMessage& msg, void* arg) {
  return impl->sendMessageInTransaction(msg, arg);
}
void TransactionMQProducer::checkTransactionState(const std::string& addr,
                                                  const MQMessageExt& message,
                                                  long tranStateTableOffset,
                                                  long commitLogOffset,
                                                  const std::string& msgId,
                                                  const std::string& transactionId,
                                                  const std::string& offsetMsgId) {
  impl->checkTransactionState(addr, message, tranStateTableOffset, commitLogOffset, msgId, transactionId, offsetMsgId);
}
}  // namespace rocketmq