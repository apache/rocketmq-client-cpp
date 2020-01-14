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
#ifndef __DEFAULT_MQ_PUSH_CONSUMER_IMPL_H__
#define __DEFAULT_MQ_PUSH_CONSUMER_IMPL_H__

#include <memory>
#include <string>
#include <thread>

#include "DefaultMQPushConsumer.h"
#include "MQClientImpl.h"
#include "MQConsumerInner.h"
#include "PullRequest.h"
#include "concurrent/executor.hpp"

namespace rocketmq {

class RebalanceImpl;
class SubscriptionData;
class OffsetStore;
class PullAPIWrapper;
class ConsumeMsgService;
class AsyncPullCallback;

class DefaultMQPushConsumerImpl;
typedef std::shared_ptr<DefaultMQPushConsumerImpl> DefaultMQPushConsumerImplPtr;

class DefaultMQPushConsumerImpl : public std::enable_shared_from_this<DefaultMQPushConsumerImpl>,
                                  public MQPushConsumer,
                                  public MQClientImpl,
                                  public MQConsumerInner {
 public:
  static DefaultMQPushConsumerImplPtr create(DefaultMQPushConsumerConfig* config, RPCHookPtr rpcHook = nullptr) {
    if (nullptr == rpcHook) {
      return DefaultMQPushConsumerImplPtr(new DefaultMQPushConsumerImpl(config));
    } else {
      return DefaultMQPushConsumerImplPtr(new DefaultMQPushConsumerImpl(config, rpcHook));
    }
  }

 private:
  DefaultMQPushConsumerImpl(DefaultMQPushConsumerConfig* config);
  DefaultMQPushConsumerImpl(DefaultMQPushConsumerConfig* config, RPCHookPtr rpcHook);

 public:
  virtual ~DefaultMQPushConsumerImpl();

 public:  // MQClient
  void start() override;
  void shutdown() override;

 public:  // MQConsumer
  bool sendMessageBack(MQMessageExt& msg, int delayLevel) override;
  bool sendMessageBack(MQMessageExt& msg, int delayLevel, const std::string& brokerName) override;
  void fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) override;

 public:  // MQPushConsumer
  void registerMessageListener(MQMessageListener* messageListener) override;
  void registerMessageListener(MessageListenerConcurrently* messageListener) override;
  void registerMessageListener(MessageListenerOrderly* messageListener) override;

  void subscribe(const std::string& topic, const std::string& subExpression) override;

  void suspend() override;
  void resume() override;

 public:  // MQConsumerInner
  std::string groupName() const override;
  MessageModel messageModel() const override;
  ConsumeType consumeType() const override;
  ConsumeFromWhere consumeFromWhere() const override;
  std::vector<SubscriptionData> subscriptions() const override;

  void doRebalance() override;
  void persistConsumerOffset() override;
  void updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info) override;
  ConsumerRunningInfo* consumerRunningInfo() override;

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

  ConsumeMsgService* getConsumerMsgService() const { return m_consumeService.get(); }

  MessageListenerType getMessageListenerType() const {
    if (nullptr != m_messageListener) {
      return m_messageListener->getMessageListenerType();
    }
    return messageListenerDefaultly;
  }

  DefaultMQPushConsumerConfig* getDefaultMQPushConsumerConfig() { return m_pushConsumerConfig; }

 private:
  void checkConfig();
  void copySubscription();
  void updateTopicSubscribeInfoWhenSubscriptionChanged();

  int getMaxReconsumeTimes();

 private:
  DefaultMQPushConsumerConfig* m_pushConsumerConfig;

  uint64_t m_startTime;

  volatile bool m_pause;
  bool m_consumeOrderly;

  std::map<std::string, std::string> m_subTopics;
  std::unique_ptr<RebalanceImpl> m_rebalanceImpl;
  std::unique_ptr<PullAPIWrapper> m_pullAPIWrapper;
  std::unique_ptr<OffsetStore> m_offsetStore;
  std::unique_ptr<ConsumeMsgService> m_consumeService;
  MQMessageListener* m_messageListener;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PUSH_CONSUMER_IMPL_H__
