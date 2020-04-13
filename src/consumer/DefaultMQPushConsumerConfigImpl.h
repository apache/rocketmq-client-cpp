
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
#ifndef __DEFAULT_MQ_PUSH_CONSUMER_CONFIG_IMPL_H__
#define __DEFAULT_MQ_PUSH_CONSUMER_CONFIG_IMPL_H__

#include "DefaultMQPushConsumerConfig.h"
#include "MQClientConfigImpl.h"

namespace rocketmq {

class DefaultMQPushConsumerConfigImpl : virtual public DefaultMQPushConsumerConfig, public MQClientConfigImpl {
 public:
  DefaultMQPushConsumerConfigImpl();
  virtual ~DefaultMQPushConsumerConfigImpl() = default;

  MessageModel getMessageModel() const override;
  void setMessageModel(MessageModel messageModel) override;

  ConsumeFromWhere getConsumeFromWhere() const override;
  void setConsumeFromWhere(ConsumeFromWhere consumeFromWhere) override;

  std::string getConsumeTimestamp() override;
  void setConsumeTimestamp(std::string consumeTimestamp) override;

  int getConsumeThreadNum() const override;
  void setConsumeThreadNum(int threadNum) override;

  int getConsumeMessageBatchMaxSize() const override;
  void setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize) override;

  int getMaxCacheMsgSizePerQueue() const override;
  void setMaxCacheMsgSizePerQueue(int maxCacheSize) override;

  int getAsyncPullTimeout() const override;
  void setAsyncPullTimeout(int asyncPullTimeout) override;

  int getMaxReconsumeTimes() const override;
  void setMaxReconsumeTimes(int maxReconsumeTimes) override;

  AllocateMQStrategy* getAllocateMQStrategy() const override;
  void setAllocateMQStrategy(AllocateMQStrategy* strategy) override;

 protected:
  MessageModel m_messageModel;

  ConsumeFromWhere m_consumeFromWhere;
  std::string m_consumeTimestamp;

  int m_consumeThreadNum;
  int m_consumeMessageBatchMaxSize;
  int m_maxMsgCacheSize;

  int m_asyncPullTimeout;  // 30s
  int m_maxReconsumeTimes;

  std::unique_ptr<AllocateMQStrategy> m_allocateMQStrategy;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PUSH_CONSUMER_CONFIG_IMPL_H__