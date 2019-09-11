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
#ifndef __DEFAULT_MQ_PULL_CONSUMER_H__
#define __DEFAULT_MQ_PULL_CONSUMER_H__

#include <set>
#include <string>

#include "AllocateMQStrategy.h"
#include "MQClient.h"
#include "MQConsumer.h"
#include "MQueueListener.h"

namespace rocketmq {

class RebalanceImpl;
class SubscriptionData;
class OffsetStore;
class PullAPIWrapper;

class ROCKETMQCLIENT_API DefaultMQPullConsumerConfig : public DefaultMQConsumerConfig {
 public:
  DefaultMQPullConsumerConfig();
  virtual ~DefaultMQPullConsumerConfig() = default;

  AllocateMQStrategy* getAllocateMQStrategy() { return m_allocateMQStrategy.get(); }
  void setAllocateMQStrategy(AllocateMQStrategy* strategy) { m_allocateMQStrategy.reset(strategy); }

 protected:
  std::unique_ptr<AllocateMQStrategy> m_allocateMQStrategy;
};

class ROCKETMQCLIENT_API DefaultMQPullConsumer : public MQPullConsumer,
                                                 public MQClient,
                                                 public DefaultMQPullConsumerConfig {
 public:
  DefaultMQPullConsumer(const std::string& groupname);
  DefaultMQPullConsumer(const std::string& groupname, RPCHookPtr rpcHook);
  virtual ~DefaultMQPullConsumer();

 public:  // MQClient
  void start() override;
  void shutdown() override;

 public:  // MQConsumer
  bool sendMessageBack(MQMessageExt& msg, int delayLevel) override;
  void fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) override;

  std::string groupName() const override;
  MessageModel messageModel() const override;
  ConsumeType consumeType() const override;
  ConsumeFromWhere consumeFromWhere() const override;
  std::vector<SubscriptionData> subscriptions() const override;

  void doRebalance() override;
  void persistConsumerOffset() override;
  void updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info) override;
  ConsumerRunningInfo* consumerRunningInfo() override { return nullptr; }

 public:  // MQPullConsumer
  void pull(const MQMessageQueue& mq,
            const std::string& subExpression,
            int64_t offset,
            int maxNums,
            PullCallback* pullCallback) override;

  /**
   * pull msg from specified queue, if no msg in queue, return directly
   *
   * @param mq
   *            specify the pulled queue
   * @param subExpression
   *            set filter expression for pulled msg, broker will filter msg actively
   *            Now only OR operation is supported, eg: "tag1 || tag2 || tag3"
   *            if subExpression is setted to "null" or "*" all msg will be subscribed
   * @param offset
   *            specify the started pull offset
   * @param maxNums
   *            specify max msg num by per pull
   * @return
   *            accroding to PullResult
   */
  PullResult pull(const MQMessageQueue& mq, const std::string& subExpression, int64_t offset, int maxNums) override;

  virtual void updateConsumeOffset(const MQMessageQueue& mq, int64_t offset);
  virtual void removeConsumeOffset(const MQMessageQueue& mq);

  void registerMessageQueueListener(const std::string& topic, MQueueListener* pListener);

  /**
   * pull msg from specified queue, if no msg, broker will suspend the pull request 20s
   *
   * @param mq
   *            specify the pulled queue
   * @param subExpression
   *            set filter expression for pulled msg, broker will filter msg actively
   *            Now only OR operation is supported, eg: "tag1 || tag2 || tag3"
   *            if subExpression is setted to "null" or "*" all msg will be subscribed
   * @param offset
   *            specify the started pull offset
   * @param maxNums
   *            specify max msg num by per pull
   * @return
   *            accroding to PullResult
   */
  PullResult pullBlockIfNotFound(const MQMessageQueue& mq,
                                 const std::string& subExpression,
                                 int64_t offset,
                                 int maxNums);

  void pullBlockIfNotFound(const MQMessageQueue& mq,
                           const std::string& subExpression,
                           int64_t offset,
                           int maxNums,
                           PullCallback* pPullCallback);

  /**
   * Fetch the offset
   *
   * @param mq
   * @param fromStore
   * @return
   */
  int64_t fetchConsumeOffset(const MQMessageQueue& mq, bool fromStore);

  /**
   * Fetch the message queues according to the topic
   *
   * @param topic Message Topic
   * @return
   */
  void fetchMessageQueuesInBalance(const std::string& topic, std::vector<MQMessageQueue> mqs);

  // temp persist consumer offset interface, only valid with
  // RemoteBrokerOffsetStore, updateConsumeOffset should be called before.
  void persistConsumerOffset4PullConsumer(const MQMessageQueue& mq);

 public:
  OffsetStore* getOffsetStore() const { return m_offsetStore.get(); }

 private:
  void checkConfig();
  void copySubscription();

  PullResult pullSyncImpl(const MQMessageQueue& mq,
                          const std::string& subExpression,
                          int64_t offset,
                          int maxNums,
                          bool block);

  void pullAsyncImpl(const MQMessageQueue& mq,
                     const std::string& subExpression,
                     int64_t offset,
                     int maxNums,
                     bool block,
                     PullCallback* pPullCallback);

  void subscriptionAutomatically(const std::string& topic);

 private:
  std::set<std::string> m_registerTopics;

  std::unique_ptr<RebalanceImpl> m_rebalanceImpl;
  std::unique_ptr<PullAPIWrapper> m_pullAPIWrapper;
  std::unique_ptr<OffsetStore> m_offsetStore;
  MQueueListener* m_messageQueueListener;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PULL_CONSUMER_H__
