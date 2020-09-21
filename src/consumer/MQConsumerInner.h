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
#ifndef ROCKETMQ_CONSUMER_MQCONSUMERINNER_H_
#define ROCKETMQ_CONSUMER_MQCONSUMERINNER_H_

#include <string>
#include <vector>

#include "ConsumeType.h"
#include "protocol/heartbeat/SubscriptionData.hpp"

namespace rocketmq {

class ConsumerRunningInfo;

class MQConsumerInner {
 public:
  virtual ~MQConsumerInner() = default;

 public:  // MQConsumerInner in Java Client
  virtual const std::string& groupName() const = 0;
  virtual MessageModel messageModel() const = 0;
  virtual ConsumeType consumeType() const = 0;
  virtual ConsumeFromWhere consumeFromWhere() const = 0;

  virtual std::vector<SubscriptionData> subscriptions() const = 0;

  // service discovery
  virtual void updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info) = 0;

  // load balancing
  virtual void doRebalance() = 0;

  // offset persistence
  virtual void persistConsumerOffset() = 0;

  virtual ConsumerRunningInfo* consumerRunningInfo() = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_MQCONSUMERINNER_H_
