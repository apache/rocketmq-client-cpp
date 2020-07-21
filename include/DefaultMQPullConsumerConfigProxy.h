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
#ifndef ROCKETMQ_DEFAULTMQPULLCONSUMERCONFIGPROXY_H_
#define ROCKETMQ_DEFAULTMQPULLCONSUMERCONFIGPROXY_H_

#include "DefaultMQPullConsumerConfig.h"
#include "MQClientConfigProxy.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultMQPullConsumerConfigProxy : virtual public DefaultMQPullConsumerConfig,
                                                            public MQClientConfigProxy {
 public:
  DefaultMQPullConsumerConfigProxy(DefaultMQPullConsumerConfigPtr consumerConfig)
      : MQClientConfigProxy(consumerConfig), m_consumerConfig(consumerConfig) {}
  virtual ~DefaultMQPullConsumerConfigProxy() = default;

  MessageModel getMessageModel() const override { return m_consumerConfig->getMessageModel(); }

  void setMessageModel(MessageModel messageModel) override { m_consumerConfig->setMessageModel(messageModel); }

  AllocateMQStrategy* getAllocateMQStrategy() const override { return m_consumerConfig->getAllocateMQStrategy(); }

  void setAllocateMQStrategy(AllocateMQStrategy* strategy) override {
    m_consumerConfig->setAllocateMQStrategy(strategy);
  }

 private:
  DefaultMQPullConsumerConfigPtr m_consumerConfig;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTMQPULLCONSUMERCONFIGPROXY_H_
