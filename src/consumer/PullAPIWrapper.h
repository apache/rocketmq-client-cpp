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
#include "protocol/heartbeat/SubscriptionData.hpp"

namespace rocketmq {

class PullAPIWrapper {
 public:
  PullAPIWrapper(MQClientInstance* instance, const std::string& consumerGroup);
  ~PullAPIWrapper();

  PullResult* processPullResult(const MQMessageQueue& mq,
                                std::unique_ptr<PullResult> pull_result,
                                SubscriptionData* subscriptionData);

  PullResult* pullKernelImpl(const MQMessageQueue& mq,
                             const std::string& subExpression,
                             const std::string& expressionType,
                             int64_t subVersion,
                             int64_t offset,
                             int maxNums,
                             int sysFlag,
                             int64_t commitOffset,
                             int brokerSuspendMaxTimeMillis,
                             int timeoutMillis,
                             CommunicationMode communicationMode,
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
