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
#include "MQClient.h"

#include "Logging.h"
#include "MQAdminImpl.h"
#include "MQClientInstance.h"
#include "MQClientManager.h"
#include "NameSpaceUtil.h"
#include "TopicPublishInfo.h"
#include "UtilAll.h"

namespace rocketmq {

#define ROCKETMQCPP_VERSION "1.0.1"
#define BUILD_DATE "03-14-2018"

// display version: strings bin/librocketmq.so |grep VERSION
const char* rocketmq_build_time = "VERSION: " ROCKETMQCPP_VERSION ", BUILD DATE: " BUILD_DATE;

void MQClient::start() {
  if (getFactory() == nullptr) {
    m_clientFactory = MQClientManager::getInstance()->getAndCreateMQClientInstance(this, m_rpcHook);
  }
  LOG_INFO_NEW("MQClient start, nameserveraddr:{}, instanceName:{}, groupName:{}, clientId:{}", getNamesrvAddr(),
               getInstanceName(), getGroupName(), m_clientFactory->getClientId());
}

void MQClient::shutdown() {
  m_clientFactory = nullptr;
}

std::vector<MQMessageQueue> MQClient::getTopicMessageQueueInfo(const std::string& topic) {
  TopicPublishInfoPtr topicPublishInfo = getFactory()->tryToFindTopicPublishInfo(topic);
  if (topicPublishInfo) {
    return topicPublishInfo->getMessageQueueList();
  }
  THROW_MQEXCEPTION(MQClientException, "could not find MessageQueue Info of topic: [" + topic + "].", -1);
}

void MQClient::createTopic(const std::string& key, const std::string& newTopic, int queueNum) {
  try {
    getFactory()->getMQAdminImpl()->createTopic(key, newTopic, queueNum);
  } catch (MQException& e) {
    LOG_ERROR(e.what());
  }
}

int64_t MQClient::searchOffset(const MQMessageQueue& mq, uint64_t timestamp) {
  return getFactory()->getMQAdminImpl()->searchOffset(mq, timestamp);
}

int64_t MQClient::maxOffset(const MQMessageQueue& mq) {
  return getFactory()->getMQAdminImpl()->maxOffset(mq);
}

int64_t MQClient::minOffset(const MQMessageQueue& mq) {
  return getFactory()->getMQAdminImpl()->minOffset(mq);
}

int64_t MQClient::earliestMsgStoreTime(const MQMessageQueue& mq) {
  return getFactory()->getMQAdminImpl()->earliestMsgStoreTime(mq);
}

MQMessageExtPtr MQClient::viewMessage(const std::string& msgId) {
  return getFactory()->getMQAdminImpl()->viewMessage(msgId);
}

QueryResult MQClient::queryMessage(const std::string& topic,
                                   const std::string& key,
                                   int maxNum,
                                   int64_t begin,
                                   int64_t end) {
  return getFactory()->getMQAdminImpl()->queryMessage(topic, key, maxNum, begin, end);
}

MQClientInstance* MQClient::getFactory() const {
  return m_clientFactory;
}

bool MQClient::isServiceStateOk() {
  return m_serviceState == RUNNING;
}

void MQClient::setLogLevel(elogLevel inputLevel) {
  ALOG_ADAPTER->setLogLevel(inputLevel);
}

elogLevel MQClient::getLogLevel() {
  return ALOG_ADAPTER->getLogLevel();
}

void MQClient::setLogFileSizeAndNum(int fileNum, long perFileSize) {
  ALOG_ADAPTER->setLogFileNumAndSize(fileNum, perFileSize);
}

}  // namespace rocketmq
