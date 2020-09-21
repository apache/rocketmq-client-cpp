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
#ifndef ROCKETMQ_MQPUSHCONSUMER_H_
#define ROCKETMQ_MQPUSHCONSUMER_H_

#include "MQMessageExt.h"
#include "MQMessageListener.h"

namespace rocketmq {

/**
 * MQPushConsumer - interface for push consumer
 */
class ROCKETMQCLIENT_API MQPushConsumer {
 public:  // MQPushConsumer in Java
  virtual void start() = 0;
  virtual void shutdown() = 0;

  virtual void suspend() = 0;
  virtual void resume() = 0;

  virtual MQMessageListener* getMessageListener() const = 0;

  virtual void registerMessageListener(MessageListenerConcurrently* messageListener) = 0;
  virtual void registerMessageListener(MessageListenerOrderly* messageListener) = 0;

  virtual void subscribe(const std::string& topic, const std::string& subExpression) = 0;
  // virtual void subscribe(const std::string& topic, MessageSelector* selector) = 0;

  virtual bool sendMessageBack(MessageExtPtr msg, int delay_level) = 0;
  virtual bool sendMessageBack(MessageExtPtr msg, int delay_level, const std::string& broker_name) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQPUSHCONSUMER_H_
