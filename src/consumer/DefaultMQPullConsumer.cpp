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

#include "DefaultMQPullConsumer.h"
#include "DefaultMQPullConsumerImpl.h"
#include "MQVersion.h"

namespace rocketmq {

DefaultMQPullConsumer::DefaultMQPullConsumer(const std::string& groupName) {
  impl = new DefaultMQPullConsumerImpl(groupName);
}

DefaultMQPullConsumer::~DefaultMQPullConsumer() {
  delete impl;
}
void DefaultMQPullConsumer::start() {
  impl->start();
}

void DefaultMQPullConsumer::shutdown() {
  impl->shutdown();
}
std::string DefaultMQPullConsumer::version() {
  std::string versions = impl->getClientVersionString();
  /*versions.append(", PROTOCOL VERSION: ")
      .append(MQVersion::GetVersionDesc(MQVersion::s_CurrentVersion))
      .append(", LANGUAGE: ")
      .append(MQVersion::s_CurrentLanguage);*/
  return versions;
}
// start mqclient set
const std::string& DefaultMQPullConsumer::getNamesrvAddr() const {
  return impl->getNamesrvAddr();
}

void DefaultMQPullConsumer::setNamesrvAddr(const std::string& namesrvAddr) {
  impl->setNamesrvAddr(namesrvAddr);
}

const std::string& DefaultMQPullConsumer::getNamesrvDomain() const {
  return impl->getNamesrvDomain();
}

void DefaultMQPullConsumer::setNamesrvDomain(const std::string& namesrvDomain) {
  impl->setNamesrvDomain(namesrvDomain);
}
void DefaultMQPullConsumer::setSessionCredentials(const std::string& accessKey,
                                                  const std::string& secretKey,
                                                  const std::string& accessChannel) {
  impl->setSessionCredentials(accessKey, secretKey, accessChannel);
}

const SessionCredentials& DefaultMQPullConsumer::getSessionCredentials() const {
  return impl->getSessionCredentials();
}
const std::string& DefaultMQPullConsumer::getInstanceName() const {
  return impl->getInstanceName();
}

void DefaultMQPullConsumer::setInstanceName(const std::string& instanceName) {
  impl->setInstanceName(instanceName);
}

const std::string& DefaultMQPullConsumer::getNameSpace() const {
  return impl->getNameSpace();
}

void DefaultMQPullConsumer::setNameSpace(const std::string& nameSpace) {
  impl->setNameSpace(nameSpace);
}
const std::string& DefaultMQPullConsumer::getGroupName() const {
  return impl->getGroupName();
}

void DefaultMQPullConsumer::setGroupName(const std::string& groupName) {
  impl->setGroupName(groupName);
}

void DefaultMQPullConsumer::setEnableSsl(bool enableSsl) {
  impl->setEnableSsl(enableSsl);
}

bool DefaultMQPullConsumer::getEnableSsl() const {
  return impl->getEnableSsl();
}

void DefaultMQPullConsumer::setSslPropertyFile(const std::string& sslPropertyFile) {
  impl->setSslPropertyFile(sslPropertyFile);
}

const std::string& DefaultMQPullConsumer::getSslPropertyFile() const {
  return impl->getSslPropertyFile();
}

void DefaultMQPullConsumer::setLogLevel(elogLevel inputLevel) {
  impl->setLogLevel(inputLevel);
}

elogLevel DefaultMQPullConsumer::getLogLevel() {
  return impl->getLogLevel();
}
void DefaultMQPullConsumer::setLogFileSizeAndNum(int fileNum, long perFileSize) {
  impl->setLogFileSizeAndNum(fileNum, perFileSize);
}

//    void DefaultMQPullConsumer::setUnitName(std::string unitName) {
//        impl->setUnitName(unitName);
//    }
//    const std::string& DefaultMQPullConsumer::getUnitName() const {
//        return impl->getUnitName();
//    }

void DefaultMQPullConsumer::fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) {
  impl->fetchSubscribeMessageQueues(topic, mqs);
}

void DefaultMQPullConsumer::persistConsumerOffset() {
  impl->persistConsumerOffset();
}
void DefaultMQPullConsumer::persistConsumerOffsetByResetOffset() {
  impl->persistConsumerOffsetByResetOffset();
}
void DefaultMQPullConsumer::updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info) {
  impl->updateTopicSubscribeInfo(topic, info);
}

ConsumeFromWhere DefaultMQPullConsumer::getConsumeFromWhere() {
  return impl->getConsumeFromWhere();
}
void DefaultMQPullConsumer::getSubscriptions(std::vector<SubscriptionData>& subData) {
  impl->getSubscriptions(subData);
}
void DefaultMQPullConsumer::updateConsumeOffset(const MQMessageQueue& mq, int64 offset) {
  impl->updateConsumeOffset(mq, offset);
}
void DefaultMQPullConsumer::removeConsumeOffset(const MQMessageQueue& mq) {
  impl->removeConsumeOffset(mq);
}

void DefaultMQPullConsumer::registerMessageQueueListener(const std::string& topic, MQueueListener* pListener) {
  impl->registerMessageQueueListener(topic, pListener);
}
PullResult DefaultMQPullConsumer::pull(const MQMessageQueue& mq,
                                       const std::string& subExpression,
                                       int64 offset,
                                       int maxNums) {
  return impl->pull(mq, subExpression, offset, maxNums);
}
void DefaultMQPullConsumer::pull(const MQMessageQueue& mq,
                                 const std::string& subExpression,
                                 int64 offset,
                                 int maxNums,
                                 PullCallback* pPullCallback) {
  impl->pull(mq, subExpression, offset, maxNums, pPullCallback);
}

PullResult DefaultMQPullConsumer::pullBlockIfNotFound(const MQMessageQueue& mq,
                                                      const std::string& subExpression,
                                                      int64 offset,
                                                      int maxNums) {
  return impl->pullBlockIfNotFound(mq, subExpression, offset, maxNums);
}
void DefaultMQPullConsumer::pullBlockIfNotFound(const MQMessageQueue& mq,
                                                const std::string& subExpression,
                                                int64 offset,
                                                int maxNums,
                                                PullCallback* pPullCallback) {
  impl->pullBlockIfNotFound(mq, subExpression, offset, maxNums, pPullCallback);
}

int64 DefaultMQPullConsumer::fetchConsumeOffset(const MQMessageQueue& mq, bool fromStore) {
  return impl->fetchConsumeOffset(mq, fromStore);
}

void DefaultMQPullConsumer::fetchMessageQueuesInBalance(const std::string& topic, std::vector<MQMessageQueue> mqs) {
  impl->fetchMessageQueuesInBalance(topic, mqs);
}
void DefaultMQPullConsumer::persistConsumerOffset4PullConsumer(const MQMessageQueue& mq) {
  // impl->persistConsumerOffsetByResetOffset(mq);
}

//<!************************************************************************
}  // namespace rocketmq
