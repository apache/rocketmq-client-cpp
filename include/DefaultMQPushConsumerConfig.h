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
#ifndef __DEFAULT_MQ_PUSH_CONSUMER_CONFIG_H__
#define __DEFAULT_MQ_PUSH_CONSUMER_CONFIG_H__

#include "AllocateMQStrategy.h"
#include "ConsumeType.h"
#include "MQClientConfig.h"

namespace rocketmq {

class DefaultMQPushConsumerConfig;
typedef std::shared_ptr<DefaultMQPushConsumerConfig> DefaultMQPushConsumerConfigPtr;

class ROCKETMQCLIENT_API DefaultMQPushConsumerConfig : virtual public MQClientConfig {
 public:
  virtual ~DefaultMQPushConsumerConfig() = default;

  virtual MessageModel getMessageModel() const = 0;
  virtual void setMessageModel(MessageModel messageModel) = 0;

  virtual ConsumeFromWhere getConsumeFromWhere() const = 0;
  virtual void setConsumeFromWhere(ConsumeFromWhere consumeFromWhere) = 0;

  virtual std::string getConsumeTimestamp() = 0;
  virtual void setConsumeTimestamp(std::string consumeTimestamp) = 0;

  /**
   * consuming thread count, default value is cpu cores
   */
  virtual int getConsumeThreadNum() const = 0;
  virtual void setConsumeThreadNum(int threadNum) = 0;

  /**
   * the pull number of message size by each pullMsg for orderly consume, default value is 1
   */
  virtual int getConsumeMessageBatchMaxSize() const = 0;
  virtual void setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize) = 0;

  /**
   * max cache msg size per Queue in memory if consumer could not consume msgs immediately,
   * default maxCacheMsgSize per Queue is 1000, set range is:1~65535
   */
  virtual int getMaxCacheMsgSizePerQueue() const = 0;
  virtual void setMaxCacheMsgSizePerQueue(int maxCacheSize) = 0;

  virtual int getAsyncPullTimeout() const = 0;
  virtual void setAsyncPullTimeout(int asyncPullTimeout) = 0;

  virtual int getMaxReconsumeTimes() const = 0;
  virtual void setMaxReconsumeTimes(int maxReconsumeTimes) = 0;

  virtual AllocateMQStrategy* getAllocateMQStrategy() const = 0;
  virtual void setAllocateMQStrategy(AllocateMQStrategy* strategy) = 0;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PUSH_CONSUMER_CONFIG_H__
