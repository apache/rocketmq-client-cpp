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
#include "MQFaultStrategy.h"

namespace rocketmq {

MQFaultStrategy::MQFaultStrategy() : m_sendLatencyFaultEnable(false) {
  m_latencyMax = std::vector<long>{50L, 100L, 550L, 1000L, 2000L, 3000L, 15000L};
  m_notAvailableDuration = std::vector<long>{0L, 0L, 30000L, 60000L, 120000L, 180000L, 600000L};
}

const MQMessageQueue& MQFaultStrategy::selectOneMessageQueue(const TopicPublishInfo* tpInfo,
                                                             const std::string& lastBrokerName) {
  if (m_sendLatencyFaultEnable) {
    {
      auto index = tpInfo->getSendWhichQueue().fetch_add(1);
      const auto& messageQueueList = tpInfo->getMessageQueueList();
      for (size_t i = 0; i < messageQueueList.size(); i++) {
        auto pos = index++ % messageQueueList.size();
        const auto& mq = messageQueueList[pos];
        if (m_latencyFaultTolerance.isAvailable(mq.broker_name())) {
          return mq;
        }
      }
    }

    auto notBestBroker = m_latencyFaultTolerance.pickOneAtLeast();
    int writeQueueNums = tpInfo->getQueueIdByBroker(notBestBroker);
    if (writeQueueNums > 0) {
      // FIXME: why modify origin mq object, not return a new one?
      static thread_local MQMessageQueue mq;
      mq = tpInfo->selectOneMessageQueue();
      if (!notBestBroker.empty()) {
        mq.set_broker_name(notBestBroker);
        mq.set_queue_id(tpInfo->getSendWhichQueue().fetch_add(1) % writeQueueNums);
      }
      return mq;
    } else {
      m_latencyFaultTolerance.remove(notBestBroker);
    }

    return tpInfo->selectOneMessageQueue();
  }

  return tpInfo->selectOneMessageQueue(lastBrokerName);
}

void MQFaultStrategy::updateFaultItem(const std::string& brokerName, const long currentLatency, bool isolation) {
  if (m_sendLatencyFaultEnable) {
    long duration = computeNotAvailableDuration(isolation ? 30000 : currentLatency);
    m_latencyFaultTolerance.updateFaultItem(brokerName, currentLatency, duration);
  }
}

long MQFaultStrategy::computeNotAvailableDuration(const long currentLatency) {
  for (size_t i = m_latencyMax.size(); i > 0; i--) {
    if (currentLatency >= m_latencyMax[i - 1]) {
      return m_notAvailableDuration[i - 1];
    }
  }
  return 0;
}

}  // namespace rocketmq
