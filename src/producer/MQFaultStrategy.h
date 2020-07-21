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
#ifndef ROCKETMQ_PRODUCER_MQFAULTSTRATEGY_H_
#define ROCKETMQ_PRODUCER_MQFAULTSTRATEGY_H_

#include <string>
#include <vector>

#include "LatencyFaultTolerancyImpl.h"
#include "MQMessageQueue.h"
#include "TopicPublishInfo.hpp"

namespace rocketmq {

class MQFaultStrategy {
 public:
  MQFaultStrategy();

  std::vector<long> getNotAvailableDuration() { return m_notAvailableDuration; }

  void setNotAvailableDuration(const std::vector<long>& notAvailableDuration) {
    m_notAvailableDuration = notAvailableDuration;
  }

  std::vector<long> getLatencyMax() { return m_latencyMax; }

  void setLatencyMax(const std::vector<long>& latencyMax) { m_latencyMax = latencyMax; }

  bool isSendLatencyFaultEnable() { return m_sendLatencyFaultEnable; }

  void setSendLatencyFaultEnable(const bool sendLatencyFaultEnable) {
    m_sendLatencyFaultEnable = sendLatencyFaultEnable;
  }

  const MQMessageQueue& selectOneMessageQueue(const TopicPublishInfo* tpInfo, const std::string& lastBrokerName);

  void updateFaultItem(const std::string& brokerName, const long currentLatency, bool isolation);

 private:
  long computeNotAvailableDuration(const long currentLatency);

 private:
  LatencyFaultTolerancyImpl m_latencyFaultTolerance;
  bool m_sendLatencyFaultEnable;

  std::vector<long> m_latencyMax;
  std::vector<long> m_notAvailableDuration;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PRODUCER_MQFAULTSTRATEGY_H_
