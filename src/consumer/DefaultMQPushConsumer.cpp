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

#include "DefaultMQPushConsumer.h"
#include <MQVersion.h>
#include "DefaultMQPushConsumerImpl.h"

namespace rocketmq {

DefaultMQPushConsumer::DefaultMQPushConsumer(const std::string& groupName) {
  impl = new DefaultMQPushConsumerImpl(groupName);
}

DefaultMQPushConsumer::~DefaultMQPushConsumer() {
  delete impl;
}
void DefaultMQPushConsumer::start() {
  impl->start();
}

void DefaultMQPushConsumer::shutdown() {
  impl->shutdown();
}
std::string DefaultMQPushConsumer::version() {
  std::string versions = impl->getClientVersionString();
  /*versions.append(", PROTOCOL VERSION: ")
      .append(MQVersion::GetVersionDesc(MQVersion::s_CurrentVersion))
      .append(", LANGUAGE: ")
      .append(MQVersion::s_CurrentLanguage);*/
  return versions;
}
// ConsumeType DefaultMQPushConsumer::getConsumeType() {
//  return impl->getConsumeType();
//}

ConsumeFromWhere DefaultMQPushConsumer::getConsumeFromWhere() {
  return impl->getConsumeFromWhere();
}
void DefaultMQPushConsumer::setConsumeFromWhere(ConsumeFromWhere consumeFromWhere) {
  impl->setConsumeFromWhere(consumeFromWhere);
}

void DefaultMQPushConsumer::registerMessageListener(MQMessageListener* pMessageListener) {
  impl->registerMessageListener(pMessageListener);
}
MessageListenerType DefaultMQPushConsumer::getMessageListenerType() {
  return impl->getMessageListenerType();
}
void DefaultMQPushConsumer::subscribe(const std::string& topic, const std::string& subExpression) {
  impl->subscribe(topic, subExpression);
}

void DefaultMQPushConsumer::setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize) {
  impl->setConsumeMessageBatchMaxSize(consumeMessageBatchMaxSize);
}
int DefaultMQPushConsumer::getConsumeMessageBatchMaxSize() const {
  return impl->getConsumeMessageBatchMaxSize();
}

/*
  set consuming thread count, default value is cpu cores
*/
void DefaultMQPushConsumer::setConsumeThreadCount(int threadCount) {
  impl->setConsumeThreadCount(threadCount);
}
int DefaultMQPushConsumer::getConsumeThreadCount() const {
  return impl->getConsumeThreadCount();
}
void DefaultMQPushConsumer::setMaxReconsumeTimes(int maxReconsumeTimes) {
  impl->setMaxReconsumeTimes(maxReconsumeTimes);
}
int DefaultMQPushConsumer::getMaxReconsumeTimes() const {
  return impl->getMaxReconsumeTimes();
}

/*
  set pullMsg thread count, default value is cpu cores
*/
void DefaultMQPushConsumer::setPullMsgThreadPoolCount(int threadCount) {
  impl->setPullMsgThreadPoolCount(threadCount);
}
int DefaultMQPushConsumer::getPullMsgThreadPoolCount() const {
  return impl->getPullMsgThreadPoolCount();
}

/*
  set max cache msg size perQueue in memory if consumer could not consume msgs
  immediately
  default maxCacheMsgSize perQueue is 1000, set range is:1~65535
*/
void DefaultMQPushConsumer::setMaxCacheMsgSizePerQueue(int maxCacheSize) {
  impl->setMaxCacheMsgSizePerQueue(maxCacheSize);
}
int DefaultMQPushConsumer::getMaxCacheMsgSizePerQueue() const {
  return impl->getMaxCacheMsgSizePerQueue();
}

MessageModel DefaultMQPushConsumer::getMessageModel() const {
  return impl->getMessageModel();
}
void DefaultMQPushConsumer::setMessageModel(MessageModel messageModel) {
  impl->setMessageModel(messageModel);
}

const std::string& DefaultMQPushConsumer::getNamesrvAddr() const {
  return impl->getNamesrvAddr();
}

void DefaultMQPushConsumer::setNamesrvAddr(const std::string& namesrvAddr) {
  impl->setNamesrvAddr(namesrvAddr);
}

const std::string& DefaultMQPushConsumer::getNamesrvDomain() const {
  return impl->getNamesrvDomain();
}

void DefaultMQPushConsumer::setNamesrvDomain(const std::string& namesrvDomain) {
  impl->setNamesrvDomain(namesrvDomain);
}
void DefaultMQPushConsumer::setSessionCredentials(const std::string& accessKey,
                                                  const std::string& secretKey,
                                                  const std::string& accessChannel) {
  impl->setSessionCredentials(accessKey, secretKey, accessChannel);
}

const SessionCredentials& DefaultMQPushConsumer::getSessionCredentials() const {
  return impl->getSessionCredentials();
}
const std::string& DefaultMQPushConsumer::getInstanceName() const {
  return impl->getInstanceName();
}

void DefaultMQPushConsumer::setInstanceName(const std::string& instanceName) {
  impl->setInstanceName(instanceName);
}

const std::string& DefaultMQPushConsumer::getNameSpace() const {
  return impl->getNameSpace();
}

void DefaultMQPushConsumer::setNameSpace(const std::string& nameSpace) {
  impl->setNameSpace(nameSpace);
}
const std::string& DefaultMQPushConsumer::getGroupName() const {
  return impl->getGroupName();
}

void DefaultMQPushConsumer::setGroupName(const std::string& groupName) {
  impl->setGroupName(groupName);
}

void DefaultMQPushConsumer::setEnableSsl(bool enableSsl) {
  impl->setEnableSsl(enableSsl);
}

bool DefaultMQPushConsumer::getEnableSsl() const {
  return impl->getEnableSsl();
}

void DefaultMQPushConsumer::setSslPropertyFile(const std::string& sslPropertyFile) {
  impl->setSslPropertyFile(sslPropertyFile);
}

const std::string& DefaultMQPushConsumer::getSslPropertyFile() const {
  return impl->getSslPropertyFile();
}

void DefaultMQPushConsumer::setLogLevel(elogLevel inputLevel) {
  impl->setLogLevel(inputLevel);
}

elogLevel DefaultMQPushConsumer::getLogLevel() {
  return impl->getLogLevel();
}
void DefaultMQPushConsumer::setLogFileSizeAndNum(int fileNum, long perFileSize) {
  impl->setLogFileSizeAndNum(fileNum, perFileSize);
}

void DefaultMQPushConsumer::setUnitName(std::string unitName) {
  impl->setUnitName(unitName);
}
const std::string& DefaultMQPushConsumer::getUnitName() const {
  return impl->getUnitName();
}

void DefaultMQPushConsumer::setTcpTransportPullThreadNum(int num) {
  impl->setTcpTransportPullThreadNum(num);
}
int DefaultMQPushConsumer::getTcpTransportPullThreadNum() const {
  return impl->getTcpTransportPullThreadNum();
}

void DefaultMQPushConsumer::setTcpTransportConnectTimeout(uint64_t timeout) {
  impl->setTcpTransportConnectTimeout(timeout);
}
uint64_t DefaultMQPushConsumer::getTcpTransportConnectTimeout() const {
  return impl->getTcpTransportConnectTimeout();
}
void DefaultMQPushConsumer::setTcpTransportTryLockTimeout(uint64_t timeout) {
  impl->setTcpTransportTryLockTimeout(timeout);
}
uint64_t DefaultMQPushConsumer::getTcpTransportTryLockTimeout() const {
  return impl->getTcpTransportTryLockTimeout();
}
void DefaultMQPushConsumer::setAsyncPull(bool asyncFlag) {
  impl->setAsyncPull(asyncFlag);
}
void DefaultMQPushConsumer::setMessageTrace(bool messageTrace) {
  impl->setMessageTrace(messageTrace);
}
bool DefaultMQPushConsumer::getMessageTrace() const {
  return impl->getMessageTrace();
}
}  // namespace rocketmq
