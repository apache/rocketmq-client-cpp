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

#include "DefaultMQProducer.h"
#include <MQVersion.h>

#include "DefaultMQProducerImpl.h"

namespace rocketmq {

DefaultMQProducer::DefaultMQProducer(const std::string& groupName) {
  impl = new DefaultMQProducerImpl(groupName);
}

DefaultMQProducer::~DefaultMQProducer() {
  delete impl;
}
void DefaultMQProducer::start() {
  impl->start();
}

void DefaultMQProducer::shutdown() {
  impl->shutdown();
}

std::string DefaultMQProducer::version() {
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
const std::string& DefaultMQProducer::getNamesrvAddr() const {
  return impl->getNamesrvAddr();
}

void DefaultMQProducer::setNamesrvAddr(const std::string& namesrvAddr) {
  impl->setNamesrvAddr(namesrvAddr);
}

const std::string& DefaultMQProducer::getNamesrvDomain() const {
  return impl->getNamesrvDomain();
}

void DefaultMQProducer::setNamesrvDomain(const std::string& namesrvDomain) {
  impl->setNamesrvDomain(namesrvDomain);
}
void DefaultMQProducer::setSessionCredentials(const std::string& accessKey,
                                              const std::string& secretKey,
                                              const std::string& accessChannel) {
  impl->setSessionCredentials(accessKey, secretKey, accessChannel);
}

const SessionCredentials& DefaultMQProducer::getSessionCredentials() const {
  return impl->getSessionCredentials();
}
const std::string& DefaultMQProducer::getInstanceName() const {
  return impl->getInstanceName();
}

void DefaultMQProducer::setInstanceName(const std::string& instanceName) {
  impl->setInstanceName(instanceName);
}

const std::string& DefaultMQProducer::getNameSpace() const {
  return impl->getNameSpace();
}

void DefaultMQProducer::setNameSpace(const std::string& nameSpace) {
  impl->setNameSpace(nameSpace);
}
const std::string& DefaultMQProducer::getGroupName() const {
  return impl->getGroupName();
}

void DefaultMQProducer::setGroupName(const std::string& groupName) {
  impl->setGroupName(groupName);
}
void DefaultMQProducer::setSendMsgTimeout(int sendMsgTimeout) {
  impl->setSendMsgTimeout(sendMsgTimeout);
}

int DefaultMQProducer::getSendMsgTimeout() const {
  return impl->getSendMsgTimeout();
}

void DefaultMQProducer::setRetryTimes(int times) {
  impl->setRetryTimes(times);
}

int DefaultMQProducer::getRetryTimes() const {
  return impl->getRetryTimes();
}

int DefaultMQProducer::getCompressMsgBodyOverHowmuch() const {
  return impl->getCompressMsgBodyOverHowmuch();
}

void DefaultMQProducer::setCompressMsgBodyOverHowmuch(int compressMsgBodyThreshold) {
  impl->setCompressMsgBodyOverHowmuch(compressMsgBodyThreshold);
}

int DefaultMQProducer::getCompressLevel() const {
  return impl->getCompressLevel();
}

void DefaultMQProducer::setCompressLevel(int compressLevel) {
  impl->setCompressLevel(compressLevel);
}

void DefaultMQProducer::setMaxMessageSize(int maxMessageSize) {
  impl->setMaxMessageSize(maxMessageSize);
}

int DefaultMQProducer::getMaxMessageSize() const {
  return impl->getMaxMessageSize();
}

void DefaultMQProducer::setRetryTimes4Async(int times) {
  impl->setRetryTimes4Async(times);
}

int DefaultMQProducer::getRetryTimes4Async() const {
  return impl->getRetryTimes4Async();
}

void DefaultMQProducer::setLogLevel(elogLevel inputLevel) {
  impl->setLogLevel(inputLevel);
}

elogLevel DefaultMQProducer::getLogLevel() {
  return impl->getLogLevel();
}
void DefaultMQProducer::setLogFileSizeAndNum(int fileNum, long perFileSize) {
  impl->setLogFileSizeAndNum(fileNum, perFileSize);
}

void DefaultMQProducer::setTcpTransportPullThreadNum(int num) {
  impl->setTcpTransportPullThreadNum(num);
}
int DefaultMQProducer::getTcpTransportPullThreadNum() const {
  return impl->getTcpTransportPullThreadNum();
}

void DefaultMQProducer::setTcpTransportConnectTimeout(uint64_t timeout) {
  impl->setTcpTransportConnectTimeout(timeout);
}
uint64_t DefaultMQProducer::getTcpTransportConnectTimeout() const {
  return impl->getTcpTransportConnectTimeout();
}
void DefaultMQProducer::setTcpTransportTryLockTimeout(uint64_t timeout) {
  impl->setTcpTransportTryLockTimeout(timeout);
}
uint64_t DefaultMQProducer::getTcpTransportTryLockTimeout() const {
  return impl->getTcpTransportTryLockTimeout();
}

void DefaultMQProducer::setUnitName(std::string unitName) {
  impl->setUnitName(unitName);
}
const std::string& DefaultMQProducer::getUnitName() const {
  return impl->getUnitName();
}
void DefaultMQProducer::setMessageTrace(bool messageTrace) {
  impl->setMessageTrace(messageTrace);
}
bool DefaultMQProducer::getMessageTrace() const {
  return impl->getMessageTrace();
}
SendResult DefaultMQProducer::send(MQMessage& msg, bool bSelectActiveBroker) {
  return impl->send(msg, bSelectActiveBroker);
}

SendResult DefaultMQProducer::send(MQMessage& msg, const MQMessageQueue& mq) {
  return impl->send(msg, mq);
}

SendResult DefaultMQProducer::send(MQMessage& msg, MessageQueueSelector* selector, void* arg) {
  return impl->send(msg, selector, arg);
}

SendResult DefaultMQProducer::send(MQMessage& msg,
                                   MessageQueueSelector* selector,
                                   void* arg,
                                   int autoRetryTimes,
                                   bool bActiveBroker) {
  return impl->send(msg, selector, arg, autoRetryTimes, bActiveBroker);
}

SendResult DefaultMQProducer::send(std::vector<MQMessage>& msgs) {
  return impl->send(msgs);
}

SendResult DefaultMQProducer::send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq) {
  return impl->send(msgs, mq);
}

void DefaultMQProducer::send(MQMessage& msg, SendCallback* pSendCallback, bool bSelectActiveBroker) {
  impl->send(msg, pSendCallback, bSelectActiveBroker);
}

void DefaultMQProducer::send(MQMessage& msg, const MQMessageQueue& mq, SendCallback* pSendCallback) {
  impl->send(msg, mq, pSendCallback);
}

void DefaultMQProducer::send(MQMessage& msg, MessageQueueSelector* selector, void* arg, SendCallback* pSendCallback) {
  impl->send(msg, selector, arg, pSendCallback);
}

void DefaultMQProducer::sendOneway(MQMessage& msg, bool bSelectActiveBroker) {
  impl->sendOneway(msg, bSelectActiveBroker);
}

void DefaultMQProducer::sendOneway(MQMessage& msg, const MQMessageQueue& mq) {
  impl->sendOneway(msg, mq);
}

void DefaultMQProducer::sendOneway(MQMessage& msg, MessageQueueSelector* selector, void* arg) {
  impl->sendOneway(msg, selector, arg);
}

void DefaultMQProducer::setEnableSsl(bool enableSsl) {
  impl->setEnableSsl(enableSsl);
}

bool DefaultMQProducer::getEnableSsl() const {
  return impl->getEnableSsl();
}

void DefaultMQProducer::setSslPropertyFile(const std::string& sslPropertyFile) {
  impl->setSslPropertyFile(sslPropertyFile);
}

const std::string& DefaultMQProducer::getSslPropertyFile() const {
  return impl->getSslPropertyFile();
}

}  // namespace rocketmq