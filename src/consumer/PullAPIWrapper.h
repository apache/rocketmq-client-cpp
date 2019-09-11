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
#ifndef __PULL_API_WRAPPER_H__
#define __PULL_API_WRAPPER_H__

#include <mutex>

#include "AsyncCallback.h"
#include "CommunicationMode.h"
#include "MQClientInstance.h"
#include "MQMessageQueue.h"
#include "SubscriptionData.h"

namespace rocketmq {

class PullAPIWrapper {
 public:
  PullAPIWrapper(MQClientInstance* mqClientFactory, const std::string& consumerGroup);
  ~PullAPIWrapper();

  PullResult processPullResult(const MQMessageQueue& mq, PullResult& pullResult, SubscriptionDataPtr subscriptionData);

  PullResult* pullKernelImpl(const MQMessageQueue& mq,             // 1
                             std::string subExpression,            // 2
                             int64_t subVersion,                   // 3
                             int64_t offset,                       // 4
                             int maxNums,                          // 5
                             int sysFlag,                          // 6
                             int64_t commitOffset,                 // 7
                             int brokerSuspendMaxTimeMillis,       // 8
                             int timeoutMillis,                    // 9
                             CommunicationMode communicationMode,  // 10
                             PullCallback* pullCallback);

 private:
  void updatePullFromWhichNode(const MQMessageQueue& mq, int brokerId);

  int recalculatePullFromWhichNode(const MQMessageQueue& mq);

 private:
  MQClientInstance* m_MQClientFactory;
  std::string m_consumerGroup;
  std::mutex m_lock;
  std::map<MQMessageQueue, int /* brokerId */> m_pullFromWhichNodeTable;
};

}  // namespace rocketmq

#endif  // __PULL_API_WRAPPER_H__
