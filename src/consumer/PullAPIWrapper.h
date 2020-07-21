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
#ifndef ROCKETMQ_CONSUMER_PULLAPIWRAPPER_H_
#define ROCKETMQ_CONSUMER_PULLAPIWRAPPER_H_

#include <mutex>  // std::mutex

#include "CommunicationMode.h"
#include "MQClientInstance.h"
#include "MQMessageQueue.h"
#include "PullCallback.h"
#include "SubscriptionData.h"

namespace rocketmq {

class PullAPIWrapper {
 public:
  PullAPIWrapper(MQClientInstance* instance, const std::string& consumerGroup);
  ~PullAPIWrapper();

  PullResult processPullResult(const MQMessageQueue& mq, PullResult& pullResult, SubscriptionData* subscriptionData);

  PullResult* pullKernelImpl(const MQMessageQueue& mq,             // 1
                             const std::string& subExpression,     // 2
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
  MQClientInstance* client_instance_;
  std::string consumer_group_;
  std::mutex lock_;
  std::map<MQMessageQueue, int /* brokerId */> pull_from_which_node_table_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_PULLAPIWRAPPER_H_
