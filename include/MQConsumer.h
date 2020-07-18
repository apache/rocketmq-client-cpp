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
#ifndef __MQ_CONSUMER_H__
#define __MQ_CONSUMER_H__

#include <string>
#include <vector>

#include "MQMessageExt.h"

namespace rocketmq {

/**
 * MQ Consumer API
 */
class ROCKETMQCLIENT_API MQConsumer {
 public:
  virtual ~MQConsumer() = default;

 public:  // MQConsumer in Java
  virtual void start() = 0;
  virtual void shutdown() = 0;

  virtual bool sendMessageBack(MessageExtPtr msg, int delayLevel) = 0;
  virtual bool sendMessageBack(MessageExtPtr msg, int delayLevel, const std::string& brokerName) = 0;
  virtual void fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) = 0;
};

}  // namespace rocketmq

#endif  // __MQ_CONSUMER_H__
