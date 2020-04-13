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
#ifndef __DEFAULT_MQ_PULL_CONSUMER_H__
#define __DEFAULT_MQ_PULL_CONSUMER_H__

#include <set>
#include <string>

#include "AllocateMQStrategy.h"
#include "DefaultMQPullConsumerConfigProxy.h"
#include "MQPullConsumer.h"
#include "RPCHook.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultMQPullConsumer : public MQPullConsumer, public DefaultMQPullConsumerConfigProxy {
 public:
  DefaultMQPullConsumer(const std::string& groupname);
  DefaultMQPullConsumer(const std::string& groupname, RPCHookPtr rpcHook);
  virtual ~DefaultMQPullConsumer();

 public:  // MQConsumer
  void start() override;
  void shutdown() override;

  bool sendMessageBack(MQMessageExt& msg, int delayLevel) override;
  bool sendMessageBack(MQMessageExt& msg, int delayLevel, const std::string& brokerName) override;
  void fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) override;

 public:  // MQPullConsumer
  void registerMessageQueueListener(const std::string& topic, MQueueListener* pListener) override;

  PullResult pull(const MQMessageQueue& mq, const std::string& subExpression, int64_t offset, int maxNums) override;

  void pull(const MQMessageQueue& mq,
            const std::string& subExpression,
            int64_t offset,
            int maxNums,
            PullCallback* pullCallback) override;

  PullResult pullBlockIfNotFound(const MQMessageQueue& mq,
                                 const std::string& subExpression,
                                 int64_t offset,
                                 int maxNums) override;

  void pullBlockIfNotFound(const MQMessageQueue& mq,
                           const std::string& subExpression,
                           int64_t offset,
                           int maxNums,
                           PullCallback* pullCallback) override;

  void updateConsumeOffset(const MQMessageQueue& mq, int64_t offset) override;

  int64_t fetchConsumeOffset(const MQMessageQueue& mq, bool fromStore) override;

  void fetchMessageQueuesInBalance(const std::string& topic, std::vector<MQMessageQueue>& mqs) override;

 public:
  void setRPCHook(RPCHookPtr rpcHook);

 protected:
  std::shared_ptr<MQPullConsumer> m_pullConsumerDelegate;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PULL_CONSUMER_H__
