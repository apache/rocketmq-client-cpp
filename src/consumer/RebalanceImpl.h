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
#ifndef ROCKETMQ_CONSUMER_REBALANCEIMPL_H_
#define ROCKETMQ_CONSUMER_REBALANCEIMPL_H_

#include <mutex>

#include "AllocateMQStrategy.h"
#include "ConsumeType.h"
#include "MQException.h"
#include "MQClientInstance.h"
#include "MQMessageQueue.h"
#include "ProcessQueue.h"
#include "PullRequest.h"
#include "protocol/heartbeat/SubscriptionData.hpp"

namespace rocketmq {

typedef std::map<MQMessageQueue, ProcessQueuePtr> MQ2PQ;
typedef std::map<std::string, std::vector<MQMessageQueue>> TOPIC2MQS;
typedef std::map<std::string, SubscriptionData*> TOPIC2SD;
typedef std::map<std::string, std::vector<MQMessageQueue>> BROKER2MQS;

class RebalanceImpl {
 public:
  RebalanceImpl(const std::string& consumerGroup,
                MessageModel messageModel,
                AllocateMQStrategy* allocateMqStrategy,
                MQClientInstance* clientInstance);
  virtual ~RebalanceImpl();

  void unlock(const MQMessageQueue& mq, const bool oneway = false);
  void unlockAll(const bool oneway = false);

  bool lock(const MQMessageQueue& mq);
  void lockAll();

  void doRebalance(const bool isOrder = false);

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
  SubscriptionData* getSubscriptionData(const std::string& topic);
  void setSubscriptionData(const std::string& topic, SubscriptionData* sd) noexcept;

  bool getTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& mqs);
  void setTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& mqs);

  void removeProcessQueue(const MQMessageQueue& mq);
  ProcessQueuePtr removeProcessQueueDirectly(const MQMessageQueue& mq);
  ProcessQueuePtr putProcessQueueIfAbsent(const MQMessageQueue& mq, ProcessQueuePtr pq);
  ProcessQueuePtr getProcessQueue(const MQMessageQueue& mq);
  MQ2PQ getProcessQueueTable();
  std::vector<MQMessageQueue> getAllocatedMQ();

 public:
  inline void set_consumer_group(const std::string& groupname) { consumer_group_ = groupname; }
  inline void set_message_model(MessageModel messageModel) { message_model_ = messageModel; }
  inline void set_allocate_mq_strategy(AllocateMQStrategy* allocateMqStrategy) {
    allocate_mq_strategy_ = allocateMqStrategy;
  }
  inline void set_client_instance(MQClientInstance* instance) { client_instance_ = instance; }

 protected:
  MQ2PQ process_queue_table_;
  std::mutex process_queue_table_mutex_;

  TOPIC2MQS topic_subscribe_info_table_;
  std::mutex topic_subscribe_info_table_mutex_;

  TOPIC2SD subscription_inner_;  // don't modify subscription_inner_ after the consumer started.

  std::string consumer_group_;
  MessageModel message_model_;
  AllocateMQStrategy* allocate_mq_strategy_;
  MQClientInstance* client_instance_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_REBALANCEIMPL_H_
