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
#ifndef __TOPIC_PUBLISH_INFO_H__
#define __TOPIC_PUBLISH_INFO_H__

#include <atomic>
#include <memory>
#include <mutex>

#include "MQMessageQueue.h"
#include "TopicRouteData.h"

namespace rocketmq {

class TopicPublishInfo;
typedef std::shared_ptr<TopicPublishInfo> TopicPublishInfoPtr;

class TopicPublishInfo {
 public:
  typedef std::vector<MQMessageQueue> QueuesVec;

 public:
  TopicPublishInfo() : m_orderTopic(false), m_haveTopicRouterInfo(false), m_sendWhichQueue(0) {}

  virtual ~TopicPublishInfo() = default;

  bool isOrderTopic() { return m_orderTopic; }

  void setOrderTopic(bool orderTopic) { m_orderTopic = orderTopic; }

  bool ok() {
    std::lock_guard<std::mutex> lock(m_queuelock);
    return !m_messageQueueList.empty();
  }

  QueuesVec& getMessageQueueList() { return m_messageQueueList; }

  std::mutex& getMessageQueueListMutex() { return m_queuelock; }

  std::atomic<std::size_t>& getSendWhichQueue() { return m_sendWhichQueue; }

  bool isHaveTopicRouterInfo() { return m_haveTopicRouterInfo; }

  void setHaveTopicRouterInfo(bool haveTopicRouterInfo) { m_haveTopicRouterInfo = haveTopicRouterInfo; }

  MQMessageQueue& selectOneMessageQueue(const std::string& lastBrokerName) {
    if (!lastBrokerName.empty()) {
      auto index = m_sendWhichQueue.fetch_add(1);
      std::lock_guard<std::mutex> lock(m_queuelock);
      for (size_t i = 0; i < m_messageQueueList.size(); i++) {
        auto pos = index++ % m_messageQueueList.size();
        auto& mq = m_messageQueueList[pos];
        if (mq.getBrokerName() != lastBrokerName) {
          return mq;
        }
      }
    }
    return selectOneMessageQueue();
  }

  MQMessageQueue& selectOneMessageQueue() {
    auto index = m_sendWhichQueue.fetch_add(1);
    std::lock_guard<std::mutex> lock(m_queuelock);
    auto pos = index % m_messageQueueList.size();
    return m_messageQueueList[pos];
  }

  int getQueueIdByBroker(const std::string& brokerName) {
    for (const auto& queueData : m_topicRouteData->getQueueDatas()) {
      if (queueData.brokerName == brokerName) {
        return queueData.writeQueueNums;
      }
    }

    return -1;
  }

  void setTopicRouteData(TopicRouteDataPtr topicRouteData) { m_topicRouteData = topicRouteData; }

 private:
  bool m_orderTopic;
  bool m_haveTopicRouterInfo;

  QueuesVec m_messageQueueList;
  std::mutex m_queuelock;

  std::atomic<std::size_t> m_sendWhichQueue;

  TopicRouteDataPtr m_topicRouteData;  // no change after set
};

}  // namespace rocketmq

#endif  // __TOPIC_PUBLISH_INFO_H__
