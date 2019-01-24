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
#ifndef __REBALANCE_IMPL_H__
#define __REBALANCE_IMPL_H__

#include <mutex>

#include "AllocateMQStrategy.h"
#include "ConsumeType.h"
#include "MQConsumer.h"
#include "MQMessageQueue.h"
#include "PullRequest.h"
#include "SubscriptionData.h"

namespace rocketmq {

class MQClientFactory;

class Rebalance {
 public:
  Rebalance(MQConsumer*, MQClientFactory*);
  virtual ~Rebalance();

  virtual void messageQueueChanged(const std::string& topic,
                                   std::vector<MQMessageQueue>& mqAll,
                                   std::vector<MQMessageQueue>& mqDivided) = 0;

  virtual void removeUnnecessaryMessageQueue(const MQMessageQueue& mq) = 0;

  virtual int64 computePullFromWhere(const MQMessageQueue& mq) = 0;

  virtual bool updateRequestTableInRebalance(const std::string& topic, std::vector<MQMessageQueue>& mqsSelf) = 0;

 public:
  void doRebalance() throw(MQClientException);
  void persistConsumerOffset();
  void persistConsumerOffsetByResetOffset();

  //<!m_subscriptionInner;
  SubscriptionData* getSubscriptionData(const std::string& topic);
  void setSubscriptionData(const std::string& topic, SubscriptionData* pdata);

  std::map<std::string, SubscriptionData*>& getSubscriptionInner();

  //<!m_topicSubscribeInfoTable;
  void setTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& mqs);
  bool getTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& mqs);

  void addPullRequest(const MQMessageQueue& mq, std::shared_ptr<PullRequest> pPullRequest);
  std::shared_ptr<PullRequest> getPullRequest(const MQMessageQueue& mq);
  std::map<MQMessageQueue, std::shared_ptr<PullRequest>> getPullRequestTable();

  void lockAll();
  bool lock(MQMessageQueue mq);
  void unlockAll(bool oneway = false);
  void unlock(MQMessageQueue mq);

 protected:
  std::map<std::string, SubscriptionData*> m_subscriptionData;

  std::mutex m_topicSubscribeInfoTableMutex;
  std::map<std::string, std::vector<MQMessageQueue>> m_topicSubscribeInfoTable;

  typedef std::map<MQMessageQueue, std::shared_ptr<PullRequest>> MQ2PULLREQ;
  MQ2PULLREQ m_requestQueueTable;
  std::mutex m_requestTableMutex;

  AllocateMQStrategy* m_pAllocateMQStrategy;
  MQConsumer* m_pConsumer;
  MQClientFactory* m_pClientFactory;
};

class RebalancePull : public Rebalance {
 public:
  RebalancePull(MQConsumer*, MQClientFactory*);

  void messageQueueChanged(const std::string& topic,
                           std::vector<MQMessageQueue>& mqAll,
                           std::vector<MQMessageQueue>& mqDivided) override;

  void removeUnnecessaryMessageQueue(const MQMessageQueue& mq) override;

  int64 computePullFromWhere(const MQMessageQueue& mq) override;

  bool updateRequestTableInRebalance(const std::string& topic, std::vector<MQMessageQueue>& mqsSelf) override;
};

class RebalancePush : public Rebalance {
 public:
  RebalancePush(MQConsumer*, MQClientFactory*);

  void messageQueueChanged(const std::string& topic,
                           std::vector<MQMessageQueue>& mqAll,
                           std::vector<MQMessageQueue>& mqDivided) override;

  void removeUnnecessaryMessageQueue(const MQMessageQueue& mq) override;

  int64 computePullFromWhere(const MQMessageQueue& mq) override;

  bool updateRequestTableInRebalance(const std::string& topic, std::vector<MQMessageQueue>& mqsSelf) override;
};

}  // namespace rocketmq

#endif  // __REBALANCE_IMPL_H__
