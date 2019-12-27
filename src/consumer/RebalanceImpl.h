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
#include "DefaultMQPullConsumer.h"
#include "DefaultMQPushConsumer.h"
#include "MQClientException.h"
#include "MQMessageQueue.h"
#include "ProcessQueue.h"
#include "PullRequest.h"
#include "SubscriptionData.h"

namespace rocketmq {

class MQClientInstance;

typedef std::map<MQMessageQueue, ProcessQueuePtr> MQ2PQ;
typedef std::map<std::string, std::vector<MQMessageQueue>> TOPIC2MQS;
typedef std::map<std::string, SubscriptionDataPtr> TOPIC2SD;
typedef std::map<std::string, std::vector<MQMessageQueue>> BROKER2MQS;

//######################################
// RebalanceImpl
//######################################

class RebalanceImpl {
 public:
  RebalanceImpl(const std::string& consumerGroup,
                MessageModel messageModel,
                AllocateMQStrategy* allocateMqStrategy,
                MQClientInstance* mqClientFactory);
  virtual ~RebalanceImpl();

  void unlock(MQMessageQueue mq, const bool oneway = false);
  void unlockAll(const bool oneway = false);

  bool lock(MQMessageQueue mq);
  void lockAll();

  void doRebalance(const bool isOrder = false) throw(MQClientException);

  void destroy();

 public:  // RebalanceImpl Interface
  virtual ConsumeType consumeType() = 0;
  virtual bool removeUnnecessaryMessageQueue(const MQMessageQueue& mq, ProcessQueuePtr pq) = 0;
  virtual void removeDirtyOffset(const MQMessageQueue& mq) = 0;
  virtual int64_t computePullFromWhere(const MQMessageQueue& mq) = 0;
  virtual void dispatchPullRequest(const std::vector<PullRequestPtr>& pullRequestList) = 0;
  virtual void messageQueueChanged(const std::string& topic,
                                   std::vector<MQMessageQueue>& mqAll,
                                   std::vector<MQMessageQueue>& mqDivided) = 0;

 private:
  std::shared_ptr<BROKER2MQS> buildProcessQueueTableByBrokerName();

  void rebalanceByTopic(const std::string& topic, const bool isOrder);
  void truncateMessageQueueNotMyTopic();
  bool updateProcessQueueTableInRebalance(const std::string& topic,
                                          std::vector<MQMessageQueue>& mqSet,
                                          const bool isOrder);

 public:
  TOPIC2SD& getSubscriptionInner();
  SubscriptionDataPtr getSubscriptionData(const std::string& topic);
  void setSubscriptionData(const std::string& topic, SubscriptionDataPtr sd) noexcept;

  bool getTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& mqs);
  void setTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& mqs);

  void removeProcessQueue(const MQMessageQueue& mq);
  ProcessQueuePtr removeProcessQueueDirectly(const MQMessageQueue& mq);
  ProcessQueuePtr putProcessQueueIfAbsent(const MQMessageQueue& mq, ProcessQueuePtr pq);
  ProcessQueuePtr getProcessQueue(const MQMessageQueue& mq);
  MQ2PQ getProcessQueueTable();
  std::vector<MQMessageQueue> getAllocatedMQ();

 public:
  void setConsumerGroup(const std::string& groupname) { m_consumerGroup = groupname; }
  void setMessageModel(MessageModel messageModel) { m_messageModel = messageModel; }
  void setAllocateMQStrategy(AllocateMQStrategy* allocateMqStrategy) { m_allocateMQStrategy = allocateMqStrategy; }
  void setMQClientFactory(MQClientInstance* instance) { m_clientInstance = instance; }

 protected:
  MQ2PQ m_processQueueTable;
  std::mutex m_processQueueTableMutex;

  TOPIC2MQS m_topicSubscribeInfoTable;
  std::mutex m_topicSubscribeInfoTableMutex;

  TOPIC2SD m_subscriptionInner;  // don't modify m_subscriptionInner after consumer start.

  std::string m_consumerGroup;
  MessageModel m_messageModel;
  AllocateMQStrategy* m_allocateMQStrategy;
  MQClientInstance* m_clientInstance;
};

//######################################
// RebalancePushImpl
//######################################

class RebalancePushImpl : public RebalanceImpl {
 public:
  RebalancePushImpl(DefaultMQPushConsumer* consumer);

  ConsumeType consumeType() override final { return CONSUME_PASSIVELY; }

  bool removeUnnecessaryMessageQueue(const MQMessageQueue& mq, ProcessQueuePtr pq) override;

  void removeDirtyOffset(const MQMessageQueue& mq) override;

  int64_t computePullFromWhere(const MQMessageQueue& mq) override;

  void dispatchPullRequest(const std::vector<PullRequestPtr>& pullRequestList) override;

  void messageQueueChanged(const std::string& topic,
                           std::vector<MQMessageQueue>& mqAll,
                           std::vector<MQMessageQueue>& mqDivided) override;

 private:
  DefaultMQPushConsumer* m_defaultMQPushConsumer;
};

//######################################
// RebalancePullImpl
//######################################

class RebalancePullImpl : public RebalanceImpl {
 public:
  RebalancePullImpl(DefaultMQPullConsumer* consumer);

  ConsumeType consumeType() override final { return CONSUME_ACTIVELY; }

  bool removeUnnecessaryMessageQueue(const MQMessageQueue& mq, ProcessQueuePtr pq) override;

  void removeDirtyOffset(const MQMessageQueue& mq) override;

  int64_t computePullFromWhere(const MQMessageQueue& mq) override;

  void dispatchPullRequest(const std::vector<PullRequestPtr>& pullRequestList) override;

  void messageQueueChanged(const std::string& topic,
                           std::vector<MQMessageQueue>& mqAll,
                           std::vector<MQMessageQueue>& mqDivided) override;

 private:
  DefaultMQPullConsumer* m_defaultMQPullConsumer;
};

}  // namespace rocketmq

#endif  // __REBALANCE_IMPL_H__
