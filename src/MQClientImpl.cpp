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
#include "MQClientImpl.h"

#include "Logging.h"
#include "MQAdminImpl.h"
#include "MQClientManager.h"
#include "TopicPublishInfo.h"
#include "UtilAll.h"

namespace rocketmq {

#define ROCKETMQCPP_VERSION "1.0.1"
#define BUILD_DATE "03-14-2018"

// display version: strings bin/librocketmq.so |grep VERSION
const char* rocketmq_build_time = "VERSION: " ROCKETMQCPP_VERSION ", BUILD DATE: " BUILD_DATE;

void MQClientImpl::start() {
  if (m_clientInstance == nullptr) {
    m_clientInstance = MQClientManager::getInstance()->getOrCreateMQClientInstance(m_clientConfig, m_rpcHook);
  }
  LOG_INFO_NEW("MQClientImpl start, nameserveraddr:{}, instanceName:{}, groupName:{}, clientId:{}",
               m_clientConfig->getNamesrvAddr(), m_clientConfig->getInstanceName(), m_clientConfig->getGroupName(),
               m_clientInstance->getClientId());
}

void MQClientImpl::shutdown() {
  m_clientInstance = nullptr;
}

void MQClientImpl::createTopic(const std::string& key, const std::string& newTopic, int queueNum) {
  try {
    m_clientInstance->getMQAdminImpl()->createTopic(key, newTopic, queueNum);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
  }
}

int64_t MQClientImpl::searchOffset(const MQMessageQueue& mq, uint64_t timestamp) {
  return m_clientInstance->getMQAdminImpl()->searchOffset(mq, timestamp);
}

int64_t MQClientImpl::maxOffset(const MQMessageQueue& mq) {
  return m_clientInstance->getMQAdminImpl()->maxOffset(mq);
}

int64_t MQClientImpl::minOffset(const MQMessageQueue& mq) {
  return m_clientInstance->getMQAdminImpl()->minOffset(mq);
}

int64_t MQClientImpl::earliestMsgStoreTime(const MQMessageQueue& mq) {
  return m_clientInstance->getMQAdminImpl()->earliestMsgStoreTime(mq);
}

MQMessageExtPtr MQClientImpl::viewMessage(const std::string& msgId) {
  return m_clientInstance->getMQAdminImpl()->viewMessage(msgId);
}

QueryResult MQClientImpl::queryMessage(const std::string& topic,
                                       const std::string& key,
                                       int maxNum,
                                       int64_t begin,
                                       int64_t end) {
  return m_clientInstance->getMQAdminImpl()->queryMessage(topic, key, maxNum, begin, end);
}

MQClientInstancePtr MQClientImpl::getFactory() const {
  return m_clientInstance;
}

bool MQClientImpl::isServiceStateOk() {
  return m_serviceState == RUNNING;
}

}  // namespace rocketmq
