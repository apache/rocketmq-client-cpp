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
#ifndef __DEFAULT_MQ_PULL_CONSUMER_CONFIG_H__
#define __DEFAULT_MQ_PULL_CONSUMER_CONFIG_H__

#include "AllocateMQStrategy.h"
#include "ConsumeType.h"
#include "MQClientConfig.h"

namespace rocketmq {

class DefaultMQPullConsumerConfig;
typedef std::shared_ptr<DefaultMQPullConsumerConfig> DefaultMQPullConsumerConfigPtr;

class ROCKETMQCLIENT_API DefaultMQPullConsumerConfig : virtual public MQClientConfig {
 public:
  virtual ~DefaultMQPullConsumerConfig() = default;

  virtual MessageModel getMessageModel() const = 0;
  virtual void setMessageModel(MessageModel messageModel) = 0;

  virtual AllocateMQStrategy* getAllocateMQStrategy() const = 0;
  virtual void setAllocateMQStrategy(AllocateMQStrategy* strategy) = 0;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PULL_CONSUMER_CONFIG_H__
