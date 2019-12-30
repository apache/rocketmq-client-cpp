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

#include "AllocateMQStrategy.h"
#include "DefaultMQConsumer.h"
#include "MQPushConsumer.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultMQPushConsumerConfig : public DefaultMQConsumerConfig {
 public:
  DefaultMQPushConsumerConfig();
  virtual ~DefaultMQPushConsumerConfig() = default;

  ConsumeFromWhere getConsumeFromWhere() const { return m_consumeFromWhere; }
  void setConsumeFromWhere(ConsumeFromWhere consumeFromWhere) { m_consumeFromWhere = consumeFromWhere; }

  /**
   * consuming thread count, default value is cpu cores
   */
  int getConsumeThreadNum() const { return m_consumeThreadNum; }
  void setConsumeThreadNum(int threadNum) {
    if (threadNum > 0) {
      m_consumeThreadNum = threadNum;
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

  int getAsyncPullTimeout() const { return m_asyncPullTimeout; }
  void setAsyncPullTimeout(int asyncPullTimeout) { m_asyncPullTimeout = asyncPullTimeout; }

  AllocateMQStrategy* getAllocateMQStrategy() { return m_allocateMQStrategy.get(); }
  void setAllocateMQStrategy(AllocateMQStrategy* strategy) { m_allocateMQStrategy.reset(strategy); }

 protected:
  ConsumeFromWhere m_consumeFromWhere;

  int m_consumeThreadNum;
  int m_consumeMessageBatchMaxSize;
  int m_maxMsgCacheSize;

  int m_asyncPullTimeout;  // 30s

  std::unique_ptr<AllocateMQStrategy> m_allocateMQStrategy;
};

class ROCKETMQCLIENT_API DefaultMQPushConsumer : public MQPushConsumer, public DefaultMQPushConsumerConfig {
 public:
  DefaultMQPushConsumer(const std::string& groupname);
  DefaultMQPushConsumer(const std::string& groupname, RPCHookPtr rpcHook);
  virtual ~DefaultMQPushConsumer();

 public:  // MQConsumer
  void start() override;
  void shutdown() override;

  bool sendMessageBack(MQMessageExt& msg, int delayLevel) override;
  void fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) override;

 public:  // MQPushConsumer
  void registerMessageListener(MQMessageListener* messageListener) override;
  void registerMessageListener(MessageListenerConcurrently* messageListener) override;
  void registerMessageListener(MessageListenerOrderly* messageListener) override;

  void subscribe(const std::string& topic, const std::string& subExpression) override;

  void suspend() override;
  void resume() override;

 public:
  void setRPCHook(std::shared_ptr<RPCHook> rpcHook);

 protected:
  std::shared_ptr<MQPushConsumer> m_pushConsumerDelegate;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PUSH_CONSUMER_H__
