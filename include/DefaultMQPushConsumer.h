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

#include "DefaultMQPushConsumerConfigProxy.h"
#include "MQPushConsumer.h"
#include "RPCHook.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultMQPushConsumer : public MQPushConsumer, public DefaultMQPushConsumerConfigProxy {
 public:
  DefaultMQPushConsumer(const std::string& groupname);
  DefaultMQPushConsumer(const std::string& groupname, RPCHookPtr rpcHook);
  virtual ~DefaultMQPushConsumer();

 public:  // MQConsumer
  void start() override;
  void shutdown() override;

  bool sendMessageBack(MQMessageExt& msg, int delayLevel) override;
  bool sendMessageBack(MQMessageExt& msg, int delayLevel, const std::string& brokerName) override;
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
