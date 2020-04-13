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
#ifndef __DEFAULT_MQ_PUSH_CONSUMER_CONFIG_PROXY_H__
#define __DEFAULT_MQ_PUSH_CONSUMER_CONFIG_PROXY_H__

#include "DefaultMQPushConsumerConfig.h"
#include "MQClientConfigProxy.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultMQPushConsumerConfigProxy : virtual public DefaultMQPushConsumerConfig,
                                                            public MQClientConfigProxy {
 public:
  DefaultMQPushConsumerConfigProxy(DefaultMQPushConsumerConfigPtr consumerConfig)
      : MQClientConfigProxy(consumerConfig), m_consumerConfig(consumerConfig) {}
  virtual ~DefaultMQPushConsumerConfigProxy() = default;

  DefaultMQPushConsumerConfigPtr getRealConfig() const { return m_consumerConfig; }

  MessageModel getMessageModel() const override { return m_consumerConfig->getMessageModel(); }

  void setMessageModel(MessageModel messageModel) override { m_consumerConfig->setMessageModel(messageModel); }

  ConsumeFromWhere getConsumeFromWhere() const override { return m_consumerConfig->getConsumeFromWhere(); }

  void setConsumeFromWhere(ConsumeFromWhere consumeFromWhere) override {
    m_consumerConfig->setConsumeFromWhere(consumeFromWhere);
  }

  std::string getConsumeTimestamp() override { return m_consumerConfig->getConsumeTimestamp(); }

  void setConsumeTimestamp(std::string consumeTimestamp) override {
    m_consumerConfig->setConsumeTimestamp(consumeTimestamp);
  }

  int getConsumeThreadNum() const override { return m_consumerConfig->getConsumeThreadNum(); }

  void setConsumeThreadNum(int threadNum) override { return m_consumerConfig->setConsumeThreadNum(threadNum); }

  int getConsumeMessageBatchMaxSize() const override { return m_consumerConfig->getConsumeMessageBatchMaxSize(); }

  void setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize) override {
    m_consumerConfig->setConsumeMessageBatchMaxSize(consumeMessageBatchMaxSize);
  }

  int getMaxCacheMsgSizePerQueue() const override { return m_consumerConfig->getMaxCacheMsgSizePerQueue(); }

  void setMaxCacheMsgSizePerQueue(int maxCacheSize) override {
    m_consumerConfig->setMaxCacheMsgSizePerQueue(maxCacheSize);
  }

  int getAsyncPullTimeout() const override { return m_consumerConfig->getAsyncPullTimeout(); }

  void setAsyncPullTimeout(int asyncPullTimeout) override { m_consumerConfig->setAsyncPullTimeout(asyncPullTimeout); }

  int getMaxReconsumeTimes() const override { return m_consumerConfig->getMaxReconsumeTimes(); }

  void setMaxReconsumeTimes(int maxReconsumeTimes) override {
    m_consumerConfig->setMaxReconsumeTimes(maxReconsumeTimes);
  }

  AllocateMQStrategy* getAllocateMQStrategy() const override { return m_consumerConfig->getAllocateMQStrategy(); }

  void setAllocateMQStrategy(AllocateMQStrategy* strategy) override {
    m_consumerConfig->setAllocateMQStrategy(strategy);
  }

 private:
  DefaultMQPushConsumerConfigPtr m_consumerConfig;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PUSH_CONSUMER_CONFIG_PROXY_H__
