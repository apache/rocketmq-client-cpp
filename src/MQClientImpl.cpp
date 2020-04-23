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

#define ROCKETMQCPP_VERSION "3.0.0"
#define BUILD_DATE __DATE__ " " __TIME__

// display version: strings bin/librocketmq.so |grep VERSION
const char* rocketmq_build_time = "VERSION: " ROCKETMQCPP_VERSION ", BUILD DATE: " BUILD_DATE;

void MQClientImpl::start() {
  if (nullptr == m_clientInstance) {
    if (nullptr == m_clientConfig) {
      THROW_MQEXCEPTION(MQClientException, "have not clientConfig for create clientInstance.", -1);
    }

    m_clientInstance = MQClientManager::getInstance()->getOrCreateMQClientInstance(*m_clientConfig, m_rpcHook);
  }

  LOG_INFO_NEW("MQClientImpl start, clientId:{}, real nameservAddr:{}", m_clientInstance->getClientId(),
               m_clientInstance->getNamesrvAddr());
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

bool MQClientImpl::isServiceStateOk() {
  return m_serviceState == RUNNING;
}

MQClientInstancePtr MQClientImpl::getClientInstance() const {
  return m_clientInstance;
}

void MQClientImpl::setClientInstance(MQClientInstancePtr clientInstance) {
  if (m_serviceState == CREATE_JUST) {
    m_clientInstance = clientInstance;
  } else {
    THROW_MQEXCEPTION(MQClientException, "Client already start, can not reset clientInstance!", -1);
  }
}

}  // namespace rocketmq
