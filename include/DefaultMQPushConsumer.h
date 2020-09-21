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
#ifndef ROCKETMQ_DEFAULTMQPUSHCONSUMER_H_
#define ROCKETMQ_DEFAULTMQPUSHCONSUMER_H_

#include "DefaultMQPushConsumerConfigProxy.h"
#include "MQPushConsumer.h"
#include "RPCHook.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultMQPushConsumer : public DefaultMQPushConsumerConfigProxy,  // base
                                                 public MQPushConsumer                     // interface
{
 public:
  DefaultMQPushConsumer(const std::string& groupname);
  DefaultMQPushConsumer(const std::string& groupname, RPCHookPtr rpcHook);

  virtual ~DefaultMQPushConsumer();

 public:  // MQPushConsumer
  void start() override;
  void shutdown() override;

  void suspend() override;
  void resume() override;

  MQMessageListener* getMessageListener() const override;

  void registerMessageListener(MessageListenerConcurrently* messageListener) override;
  void registerMessageListener(MessageListenerOrderly* messageListener) override;

  void subscribe(const std::string& topic, const std::string& subExpression) override;

  bool sendMessageBack(MessageExtPtr msg, int delayLevel) override;
  bool sendMessageBack(MessageExtPtr msg, int delayLevel, const std::string& brokerName) override;

 public:
  void setRPCHook(RPCHookPtr rpcHook);

 protected:
  std::shared_ptr<MQPushConsumer> push_consumer_impl_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTMQPUSHCONSUMER_H_
