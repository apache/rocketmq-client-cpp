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
#ifndef ROCKETMQ_CONSUMER_REBALANCELITEPULLIMPL_H_
#define ROCKETMQ_CONSUMER_REBALANCELITEPULLIMPL_H_

#include "DefaultLitePullConsumerImpl.h"
#include "RebalanceImpl.h"

namespace rocketmq {

typedef std::map<MQMessageQueue, ProcessQueuePtr> MQ2PQ;
typedef std::map<std::string, std::vector<MQMessageQueue>> TOPIC2MQS;
typedef std::map<std::string, SubscriptionData*> TOPIC2SD;
typedef std::map<std::string, std::vector<MQMessageQueue>> BROKER2MQS;

class RebalanceLitePullImpl : public RebalanceImpl {
 public:
  RebalanceLitePullImpl(DefaultLitePullConsumerImpl* consumerImpl);

  ConsumeType consumeType() override final { return CONSUME_ACTIVELY; }

  bool removeUnnecessaryMessageQueue(const MQMessageQueue& mq, ProcessQueuePtr pq) override;

  void removeDirtyOffset(const MQMessageQueue& mq) override;

  int64_t computePullFromWhere(const MQMessageQueue& mq) override;

  void dispatchPullRequest(const std::vector<PullRequestPtr>& pullRequestList) override;

  void messageQueueChanged(const std::string& topic,
                           std::vector<MQMessageQueue>& mqAll,
                           std::vector<MQMessageQueue>& mqDivided) override;

 private:
  DefaultLitePullConsumerImpl* lite_pull_consumer_impl_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_REBALANCELITEPULLIMPL_H_
