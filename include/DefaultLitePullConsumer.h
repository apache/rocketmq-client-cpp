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
#ifndef ROCKETMQ_DEFAULTLITEPULLCONSUMER_H_
#define ROCKETMQ_DEFAULTLITEPULLCONSUMER_H_

#include "DefaultLitePullConsumerConfigProxy.h"
#include "LitePullConsumer.h"
#include "RPCHook.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultLitePullConsumer : public DefaultLitePullConsumerConfigProxy,  // base
                                                   public LitePullConsumer                     // interface
{
 public:
  DefaultLitePullConsumer(const std::string& groupname);
  DefaultLitePullConsumer(const std::string& groupname, RPCHookPtr rpcHook);
  virtual ~DefaultLitePullConsumer();

 public:  // LitePullConsumer
  void start() override;
  void shutdown() override;

  bool isAutoCommit() const override;
  void setAutoCommit(bool auto_commit) override;

  void subscribe(const std::string& topic, const std::string& subExpression) override;
  void subscribe(const std::string& topic, const MessageSelector& selector) override;

  void unsubscribe(const std::string& topic) override;

  std::vector<MQMessageExt> poll() override;
  std::vector<MQMessageExt> poll(long timeout) override;

  std::vector<MQMessageQueue> fetchMessageQueues(const std::string& topic) override;

  void assign(const std::vector<MQMessageQueue>& messageQueues) override;

  void seek(const MQMessageQueue& messageQueue, int64_t offset) override;
  void seekToBegin(const MQMessageQueue& messageQueue) override;
  void seekToEnd(const MQMessageQueue& messageQueue) override;

  int64_t offsetForTimestamp(const MQMessageQueue& messageQueue, int64_t timestamp) override;

  void pause(const std::vector<MQMessageQueue>& messageQueues) override;
  void resume(const std::vector<MQMessageQueue>& messageQueues) override;

  void commitSync() override;

  int64_t committed(const MQMessageQueue& messageQueue) override;

  void registerTopicMessageQueueChangeListener(
      const std::string& topic,
      TopicMessageQueueChangeListener* topicMessageQueueChangeListener) override;

 public:
  void setRPCHook(RPCHookPtr rpcHook);

 protected:
  std::shared_ptr<LitePullConsumer> pull_consumer_impl_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTLITEPULLCONSUMER_H_
