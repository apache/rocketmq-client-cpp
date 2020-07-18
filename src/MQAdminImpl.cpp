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
#include "MQAdminImpl.h"

#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "TopicPublishInfo.h"

namespace rocketmq {

void MQAdminImpl::createTopic(const std::string& key, const std::string& newTopic, int queueNum) {}

void MQAdminImpl::fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) {
  try {
    TopicRouteDataPtr topicRouteData(
        m_clientInstance->getMQClientAPIImpl()->getTopicRouteInfoFromNameServer(topic, 1000 * 3));
    if (topicRouteData != nullptr) {
      mqs = m_clientInstance->topicRouteData2TopicSubscribeInfo(topic, topicRouteData);
      if (!mqs.empty()) {
        return;
      } else {
        THROW_MQEXCEPTION(MQClientException,
                          "Can not find Message Queue for this topic, " + topic + " Namesrv return empty", -1);
      }
    }
  } catch (const std::exception& e) {
    THROW_MQEXCEPTION(MQClientException, "Can not find Message Queue for this topic, " + topic, -1);
  }

  THROW_MQEXCEPTION(MQClientException, "Unknown why, Can not find Message Queue for this topic, " + topic, -1);
}

int64_t MQAdminImpl::searchOffset(const MQMessageQueue& mq, int64_t timestamp) {
  std::string brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  if (brokerAddr.empty()) {
    m_clientInstance->updateTopicRouteInfoFromNameServer(mq.getTopic());
    brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  }

  if (!brokerAddr.empty()) {
    try {
      return m_clientInstance->getMQClientAPIImpl()->searchOffset(brokerAddr, mq.getTopic(), mq.getQueueId(), timestamp,
                                                                  1000 * 3);
    } catch (MQException& e) {
      THROW_MQEXCEPTION(MQClientException, "Invoke Broker exception", -1);
    }
  }
  THROW_MQEXCEPTION(MQClientException, "The broker is not exist", -1);
}

int64_t MQAdminImpl::maxOffset(const MQMessageQueue& mq) {
  std::string brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  if (brokerAddr.empty()) {
    m_clientInstance->updateTopicRouteInfoFromNameServer(mq.getTopic());
    brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  }

  if (!brokerAddr.empty()) {
    try {
      return m_clientInstance->getMQClientAPIImpl()->getMaxOffset(brokerAddr, mq.getTopic(), mq.getQueueId(), 1000 * 3);
    } catch (MQException& e) {
      THROW_MQEXCEPTION(MQClientException, "Invoke Broker exception", -1);
    }
  }
  THROW_MQEXCEPTION(MQClientException, "The broker is not exist", -1);
}

int64_t MQAdminImpl::minOffset(const MQMessageQueue& mq) {
  std::string brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  if (brokerAddr.empty()) {
    m_clientInstance->updateTopicRouteInfoFromNameServer(mq.getTopic());
    brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  }

  if (!brokerAddr.empty()) {
    try {
      return m_clientInstance->getMQClientAPIImpl()->getMinOffset(brokerAddr, mq.getTopic(), mq.getQueueId(), 1000 * 3);
    } catch (const std::exception& e) {
      THROW_MQEXCEPTION(MQClientException, "Invoke Broker[" + brokerAddr + "] exception", -1);
    }
  }

  THROW_MQEXCEPTION(MQClientException, "The broker[" + mq.getBrokerName() + "] not exist", -1);
}

int64_t MQAdminImpl::earliestMsgStoreTime(const MQMessageQueue& mq) {
  std::string brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  if (brokerAddr.empty()) {
    m_clientInstance->updateTopicRouteInfoFromNameServer(mq.getTopic());
    brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  }

  if (!brokerAddr.empty()) {
    try {
      return m_clientInstance->getMQClientAPIImpl()->getEarliestMsgStoretime(brokerAddr, mq.getTopic(), mq.getQueueId(),
                                                                             1000 * 3);
    } catch (MQException& e) {
      THROW_MQEXCEPTION(MQClientException, "Invoke Broker exception", -1);
    }
  }
  THROW_MQEXCEPTION(MQClientException, "The broker is not exist", -1);
}

MQMessageExt MQAdminImpl::viewMessage(const std::string& msgId) {
  try {
    return MQMessageExt(nullptr);
  } catch (MQException& e) {
    THROW_MQEXCEPTION(MQClientException, "message id illegal", -1);
  }
}

QueryResult MQAdminImpl::queryMessage(const std::string& topic,
                                      const std::string& key,
                                      int maxNum,
                                      int64_t begin,
                                      int64_t end) {
  THROW_MQEXCEPTION(MQClientException, "queryMessage", -1);
}

}  // namespace rocketmq
