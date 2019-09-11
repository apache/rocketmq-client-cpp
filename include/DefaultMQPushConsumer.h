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
#ifndef __DEFAULT_MQ_PUSH_CONSUMER_H__
#define __DEFAULT_MQ_PUSH_CONSUMER_H__

#include <memory>
#include <string>
#include <thread>

#include "concurrent/executor.hpp"

#include "AllocateMQStrategy.h"
#include "MQClient.h"
#include "MQConsumer.h"
#include "PullRequest.h"

namespace rocketmq {

class RebalanceImpl;
class SubscriptionData;
class OffsetStore;
class PullAPIWrapper;
class ConsumeMsgService;
class AsyncPullCallback;

class ROCKETMQCLIENT_API DefaultMQPushConsumerConfig : public DefaultMQConsumerConfig {
 public:
  DefaultMQPushConsumerConfig();
  virtual ~DefaultMQPushConsumerConfig() = default;

  ConsumeFromWhere getConsumeFromWhere() const { return m_consumeFromWhere; }
  void setConsumeFromWhere(ConsumeFromWhere consumeFromWhere) { m_consumeFromWhere = consumeFromWhere; }

  /**
   * consuming thread count, default value is cpu cores
   */
  int getConsumeThreadCount() const { return m_consumeThreadCount; }
  void setConsumeThreadCount(int threadCount) {
    if (threadCount > 0) {
      m_consumeThreadCount = threadCount;
    }
  }

  /**
   * the pull number of message size by each pullMsg for orderly consume, default value is 1
   */
  int getConsumeMessageBatchMaxSize() const { return m_consumeMessageBatchMaxSize; }
  void setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize) {
    if (consumeMessageBatchMaxSize >= 1) {
      m_consumeMessageBatchMaxSize = consumeMessageBatchMaxSize;
    }
  }

  /**
   * max cache msg size per Queue in memory if consumer could not consume msgs immediately,
   * default maxCacheMsgSize per Queue is 1000, set range is:1~65535
   */
  int getMaxCacheMsgSizePerQueue() const { return m_maxMsgCacheSize; }
  void setMaxCacheMsgSizePerQueue(int maxCacheSize) {
    if (maxCacheSize > 0 && maxCacheSize < 65535) {
      m_maxMsgCacheSize = maxCacheSize;
    }
  }

  AllocateMQStrategy* getAllocateMQStrategy() { return m_allocateMQStrategy.get(); }
  void setAllocateMQStrategy(AllocateMQStrategy* strategy) { m_allocateMQStrategy.reset(strategy); }

 protected:
  ConsumeFromWhere m_consumeFromWhere;

  int m_consumeThreadCount;
  int m_consumeMessageBatchMaxSize;
  int m_maxMsgCacheSize;

  int m_asyncPullTimeout;  // 30s

  std::unique_ptr<AllocateMQStrategy> m_allocateMQStrategy;
};

class ROCKETMQCLIENT_API DefaultMQPushConsumer : public MQPushConsumer,
                                                 public MQClient,
                                                 public DefaultMQPushConsumerConfig {
 public:
  DefaultMQPushConsumer(const std::string& groupname);
  DefaultMQPushConsumer(const std::string& groupname, RPCHookPtr rpcHook);
  virtual ~DefaultMQPushConsumer();

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
  ConsumerRunningInfo* consumerRunningInfo() override;

 public:  // MQPushConsumer
  void registerMessageListener(MQMessageListener* messageListener) override;
  void registerMessageListener(MessageListenerConcurrently* messageListener) override;
  void registerMessageListener(MessageListenerOrderly* messageListener) override;

  void subscribe(const std::string& topic, const std::string& subExpression) override;

  void suspend() override;
  void resume() override;

 public:
  bool isPause();
  void setPause(bool pause);

  void updateConsumeOffset(const MQMessageQueue& mq, int64_t offset);
  void correctTagsOffset(PullRequestPtr pullRequest);

  void executePullRequestLater(PullRequestPtr pullRequest, long timeDelay);
  void executePullRequestImmediately(PullRequestPtr pullRequest);
  void executeTaskLater(const handler_type& task, long timeDelay);

  void pullMessage(PullRequestPtr pullrequest);

  void resetRetryTopic(std::vector<MQMessageExtPtr2>& msgs, const std::string& consumerGroup);

 public:
  bool isConsumeOrderly() { return m_consumeOrderly; }
  RebalanceImpl* getRebalanceImpl() const { return m_rebalanceImpl.get(); }
  PullAPIWrapper* getPullAPIWrapper() const { return m_pullAPIWrapper.get(); }
  OffsetStore* getOffsetStore() const { return m_offsetStore.get(); }
  ConsumeMsgService* getConsumerMsgService() const { return m_consumerService.get(); }
  MessageListenerType getMessageListenerType() const {
    if (nullptr != m_messageListener) {
      return m_messageListener->getMessageListenerType();
    }
    return messageListenerDefaultly;
  }

 private:
  void checkConfig();
  void copySubscription();
  void updateTopicSubscribeInfoWhenSubscriptionChanged();

 private:
  uint64_t m_startTime;

  volatile bool m_pause;
  bool m_consumeOrderly;

  std::map<std::string, std::string> m_subTopics;
  std::unique_ptr<RebalanceImpl> m_rebalanceImpl;
  std::unique_ptr<PullAPIWrapper> m_pullAPIWrapper;
  std::unique_ptr<OffsetStore> m_offsetStore;
  std::unique_ptr<ConsumeMsgService> m_consumerService;
  MQMessageListener* m_messageListener;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PUSH_CONSUMER_H__
