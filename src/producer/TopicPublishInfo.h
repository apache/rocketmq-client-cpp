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

#include "MQClientException.h"
#include "MQMessageQueue.h"
#include "TopicRouteData.h"

namespace rocketmq {

class TopicPublishInfo;
typedef std::shared_ptr<const TopicPublishInfo> TopicPublishInfoPtr;

class TopicPublishInfo {
 public:
  typedef std::vector<MQMessageQueue> QueuesVec;

 public:
  TopicPublishInfo() : m_orderTopic(false), m_haveTopicRouterInfo(false), m_sendWhichQueue(0) {}

  virtual ~TopicPublishInfo() = default;

  bool isOrderTopic() const { return m_orderTopic; }

  void setOrderTopic(bool orderTopic) { m_orderTopic = orderTopic; }

  bool ok() const { return !m_messageQueueList.empty(); }

  const QueuesVec& getMessageQueueList() const { return m_messageQueueList; }

  std::atomic<std::size_t>& getSendWhichQueue() const { return m_sendWhichQueue; }

  bool isHaveTopicRouterInfo() { return m_haveTopicRouterInfo; }

  void setHaveTopicRouterInfo(bool haveTopicRouterInfo) { m_haveTopicRouterInfo = haveTopicRouterInfo; }

  const MQMessageQueue& selectOneMessageQueue(const std::string& lastBrokerName) const {
    if (!lastBrokerName.empty()) {
      auto mqSize = m_messageQueueList.size();
      if (mqSize <= 1) {
        if (mqSize == 0) {
          LOG_ERROR_NEW("FIXME: messageQueueList is empty");
          THROW_MQEXCEPTION(MQClientException, "messageQueueList is empty", -1);
        }
        return m_messageQueueList[0];
      } else {
        // NOTE: If it possible, mq in same broker is nonadjacent.
        auto index = m_sendWhichQueue.fetch_add(1);
        for (size_t i = 0; i < 2; i++) {
          auto pos = index++ % m_messageQueueList.size();
          auto& mq = m_messageQueueList[pos];
          if (mq.getBrokerName() != lastBrokerName) {
            return mq;
          }
        }
        return m_messageQueueList[(index - 2) % m_messageQueueList.size()];
      }
    }
    return selectOneMessageQueue();
  }

  const MQMessageQueue& selectOneMessageQueue() const {
    auto index = m_sendWhichQueue.fetch_add(1);
    auto pos = index % m_messageQueueList.size();
    return m_messageQueueList[pos];
  }

  int getQueueIdByBroker(const std::string& brokerName) const {
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

  QueuesVec m_messageQueueList;  // no change after build
  mutable std::atomic<std::size_t> m_sendWhichQueue;

  TopicRouteDataPtr m_topicRouteData;  // no change after set
};

}  // namespace rocketmq

#endif  // __TOPIC_PUBLISH_INFO_H__
