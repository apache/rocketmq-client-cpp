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
#ifndef ROCKETMQ_DEFAULTMQPUSHCONSUMERCONFIGPROXY_H_
#define ROCKETMQ_DEFAULTMQPUSHCONSUMERCONFIGPROXY_H_

#include "DefaultMQPushConsumerConfig.h"
#include "MQClientConfigProxy.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultMQPushConsumerConfigProxy : public MQClientConfigProxy,                 // base
                                                            virtual public DefaultMQPushConsumerConfig  // interface
{
 public:
  DefaultMQPushConsumerConfigProxy(DefaultMQPushConsumerConfigPtr consumerConfig)
      : MQClientConfigProxy(consumerConfig) {}
  virtual ~DefaultMQPushConsumerConfigProxy() = default;

  inline MessageModel getMessageModel() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMessageModel();
  }

  inline void setMessageModel(MessageModel messageModel) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->setMessageModel(messageModel);
  }

  inline ConsumeFromWhere getConsumeFromWhere() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getConsumeFromWhere();
  }

  inline void setConsumeFromWhere(ConsumeFromWhere consumeFromWhere) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->setConsumeFromWhere(consumeFromWhere);
  }

  inline const std::string& getConsumeTimestamp() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getConsumeTimestamp();
  }

  inline void setConsumeTimestamp(const std::string& consumeTimestamp) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->setConsumeTimestamp(consumeTimestamp);
  }

  inline int getConsumeThreadNum() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getConsumeThreadNum();
  }

  inline void setConsumeThreadNum(int threadNum) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->setConsumeThreadNum(threadNum);
  }

  inline int getConsumeMessageBatchMaxSize() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getConsumeMessageBatchMaxSize();
  }

  inline void setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())
        ->setConsumeMessageBatchMaxSize(consumeMessageBatchMaxSize);
  }

  inline int getMaxCacheMsgSizePerQueue() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMaxCacheMsgSizePerQueue();
  }

  inline void setMaxCacheMsgSizePerQueue(int maxCacheSize) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->setMaxCacheMsgSizePerQueue(maxCacheSize);
  }

  inline int getAsyncPullTimeout() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getAsyncPullTimeout();
  }

  inline void setAsyncPullTimeout(int asyncPullTimeout) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->setAsyncPullTimeout(asyncPullTimeout);
  }

  inline int getMaxReconsumeTimes() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getMaxReconsumeTimes();
  }

  inline void setMaxReconsumeTimes(int maxReconsumeTimes) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->setMaxReconsumeTimes(maxReconsumeTimes);
  }

  inline long getPullTimeDelayMillsWhenException() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getPullTimeDelayMillsWhenException();
  }

  inline void setPullTimeDelayMillsWhenException(long pullTimeDelayMillsWhenException) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())
        ->setPullTimeDelayMillsWhenException(pullTimeDelayMillsWhenException);
  }

  inline AllocateMQStrategy* getAllocateMQStrategy() const override {
    return dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->getAllocateMQStrategy();
  }

  inline void setAllocateMQStrategy(AllocateMQStrategy* strategy) override {
    dynamic_cast<DefaultMQPushConsumerConfig*>(client_config_.get())->setAllocateMQStrategy(strategy);
  }

  inline DefaultMQPushConsumerConfigPtr real_config() const {
    return std::dynamic_pointer_cast<DefaultMQPushConsumerConfig>(client_config_);
  }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTMQPUSHCONSUMERCONFIGPROXY_H_
