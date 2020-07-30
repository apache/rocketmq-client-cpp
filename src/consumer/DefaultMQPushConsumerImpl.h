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
#ifndef ROCKETMQ_CONXUMER_DEFAULTMQPUSHCONSUMERIMPL_H_
#define ROCKETMQ_CONXUMER_DEFAULTMQPUSHCONSUMERIMPL_H_

#include <memory>
#include <string>
#include <thread>

#include "concurrent/executor.hpp"
#include "DefaultMQPushConsumer.h"
#include "MQClientImpl.h"
#include "MQConsumerInner.h"
#include "PullRequest.h"

namespace rocketmq {

class AsyncPullCallback;
class ConsumeMsgService;
class OffsetStore;
class PullAPIWrapper;
class RebalanceImpl;
class SubscriptionData;

class DefaultMQPushConsumerImpl;
typedef std::shared_ptr<DefaultMQPushConsumerImpl> DefaultMQPushConsumerImplPtr;

class DefaultMQPushConsumerImpl : public std::enable_shared_from_this<DefaultMQPushConsumerImpl>,
                                  public MQPushConsumer,
                                  public MQClientImpl,
                                  public MQConsumerInner {
 public:
  /**
   * create() - Factory method for DefaultMQPushConsumerImpl, used to ensure that all objects of
   * DefaultMQPushConsumerImpl are managed by std::share_ptr
   */
  static DefaultMQPushConsumerImplPtr create(DefaultMQPushConsumerConfigPtr config, RPCHookPtr rpcHook = nullptr) {
    if (nullptr == rpcHook) {
      return DefaultMQPushConsumerImplPtr(new DefaultMQPushConsumerImpl(config));
    } else {
      return DefaultMQPushConsumerImplPtr(new DefaultMQPushConsumerImpl(config, rpcHook));
    }
  }

 private:
  DefaultMQPushConsumerImpl(DefaultMQPushConsumerConfigPtr config);
  DefaultMQPushConsumerImpl(DefaultMQPushConsumerConfigPtr config, RPCHookPtr rpcHook);

 public:
  virtual ~DefaultMQPushConsumerImpl();

 public:  // MQClient
  void start() override;
  void shutdown() override;

 public:  // MQConsumer
  bool sendMessageBack(MessageExtPtr msg, int delayLevel) override;
  bool sendMessageBack(MessageExtPtr msg, int delayLevel, const std::string& brokerName) override;
  void fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) override;

 public:  // MQPushConsumer
  void registerMessageListener(MQMessageListener* messageListener) override;
  void registerMessageListener(MessageListenerConcurrently* messageListener) override;
  void registerMessageListener(MessageListenerOrderly* messageListener) override;

  MQMessageListener* getMessageListener() const override;

  void subscribe(const std::string& topic, const std::string& subExpression) override;

  void suspend() override;
  void resume() override;

 public:  // MQConsumerInner
  const std::string& groupName() const override;
  MessageModel messageModel() const override;
  ConsumeType consumeType() const override;
  ConsumeFromWhere consumeFromWhere() const override;
  std::vector<SubscriptionData> subscriptions() const override;

  void doRebalance() override;
  void persistConsumerOffset() override;
  void updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info) override;

  ConsumerRunningInfo* consumerRunningInfo() override;

 public:
  void updateConsumeOffset(const MQMessageQueue& mq, int64_t offset);
  void correctTagsOffset(PullRequestPtr pullRequest);

  void executePullRequestLater(PullRequestPtr pullRequest, long timeDelay);
  void executePullRequestImmediately(PullRequestPtr pullRequest);
  void executeTaskLater(const handler_type& task, long timeDelay);

  void pullMessage(PullRequestPtr pullrequest);

  void resetRetryTopic(const std::vector<MessageExtPtr>& msgs, const std::string& consumerGroup);

 private:
  void checkConfig();
  void copySubscription();
  void updateTopicSubscribeInfoWhenSubscriptionChanged();

 public:
  int getMaxReconsumeTimes();

  inline bool isPause() const { return pause_; };
  inline void setPause(bool pause) { pause_ = pause; }

  inline bool isConsumeOrderly() { return consume_orderly_; }

  inline RebalanceImpl* getRebalanceImpl() const { return rebalance_impl_.get(); }

  inline PullAPIWrapper* getPullAPIWrapper() const { return pull_api_wrapper_.get(); }

  inline OffsetStore* getOffsetStore() const { return offset_store_.get(); }

  inline ConsumeMsgService* getConsumerMsgService() const { return consume_service_.get(); }

  inline MessageListenerType getMessageListenerType() const {
    if (nullptr != message_listener_) {
      return message_listener_->getMessageListenerType();
    }
    return messageListenerDefaultly;
  }

  inline DefaultMQPushConsumerConfig* getDefaultMQPushConsumerConfig() {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get());
  }

 private:
  uint64_t start_time_;

  volatile bool pause_;
  bool consume_orderly_;

  std::map<std::string, std::string> subscription_;
  std::unique_ptr<RebalanceImpl> rebalance_impl_;
  std::unique_ptr<PullAPIWrapper> pull_api_wrapper_;
  std::unique_ptr<OffsetStore> offset_store_;
  std::unique_ptr<ConsumeMsgService> consume_service_;
  MQMessageListener* message_listener_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONXUMER_DEFAULTMQPUSHCONSUMERIMPL_H_
