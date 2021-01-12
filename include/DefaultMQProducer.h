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
#ifndef ROCKETMQ_DEFAULTMQPRODUCER_H_
#define ROCKETMQ_DEFAULTMQPRODUCER_H_

#include "DefaultMQProducerConfigProxy.h"
#include "MQProducer.h"
#include "RPCHook.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultMQProducer : public DefaultMQProducerConfigProxy,  // base
                                             public MQProducer                     // interface
{
 public:
  DefaultMQProducer(const std::string& groupname);
  DefaultMQProducer(const std::string& groupname, RPCHookPtr rpcHook);
  virtual ~DefaultMQProducer();

 private:
  DefaultMQProducer(const std::string& groupname, RPCHookPtr rpcHook, DefaultMQProducerConfigPtr producerConfig);
  friend class TransactionMQProducer;

 public:  // MQProducer
  void start() override;
  void shutdown() override;

  std::vector<MQMessageQueue> fetchPublishMessageQueues(const std::string& topic) override;

  // Sync: caller will be responsible for the lifecycle of messages.
  SendResult send(MQMessage& msg) override;
  SendResult send(MQMessage& msg, long timeout) override;
  SendResult send(MQMessage& msg, const MQMessageQueue& mq) override;
  SendResult send(MQMessage& msg, const MQMessageQueue& mq, long timeout) override;

  // Async: don't delete msg object, until callback occur.
  void send(MQMessage& msg, SendCallback* sendCallback) noexcept override;
  void send(MQMessage& msg, SendCallback* sendCallback, long timeout) noexcept override;
  void send(MQMessage& msg, const MQMessageQueue& mq, SendCallback* sendCallback) noexcept override;
  void send(MQMessage& msg, const MQMessageQueue& mq, SendCallback* sendCallback, long timeout) noexcept override;

  // Oneyway: same as sync send, but don't care its result.
  void sendOneway(MQMessage& msg) override;
  void sendOneway(MQMessage& msg, const MQMessageQueue& mq) override;

  // Select
  SendResult send(MQMessage& msg, MessageQueueSelector* selector, void* arg) override;
  SendResult send(MQMessage& msg, MessageQueueSelector* selector, void* arg, long timeout) override;
  void send(MQMessage& msg, MessageQueueSelector* selector, void* arg, SendCallback* sendCallback) noexcept override;
  void send(MQMessage& msg,
            MessageQueueSelector* selector,
            void* arg,
            SendCallback* sendCallback,
            long timeout) noexcept override;
  void sendOneway(MQMessage& msg, MessageQueueSelector* selector, void* arg) override;

  // Transaction
  TransactionSendResult sendMessageInTransaction(MQMessage& msg, void* arg) override;

  // Batch: power by sync send, caller will be responsible for the lifecycle of messages.
  SendResult send(std::vector<MQMessage>& msgs) override;
  SendResult send(std::vector<MQMessage>& msgs, long timeout) override;
  SendResult send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq) override;
  SendResult send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq, long timeout) override;

  void send(std::vector<MQMessage>& msgs, SendCallback* sendCallback) override;
  void send(std::vector<MQMessage>& msgs, SendCallback* sendCallback, long timeout) override;
  void send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq, SendCallback* sendCallback) override;
  void send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq, SendCallback* sendCallback, long timeout) override;

  // RPC
  MQMessage request(MQMessage& msg, long timeout) override;
  void request(MQMessage& msg, RequestCallback* requestCallback, long timeout) override;
  MQMessage request(MQMessage& msg, const MQMessageQueue& mq, long timeout) override;
  void request(MQMessage& msg, const MQMessageQueue& mq, RequestCallback* requestCallback, long timeout) override;
  MQMessage request(MQMessage& msg, MessageQueueSelector* selector, void* arg, long timeout) override;
  void request(MQMessage& msg,
               MessageQueueSelector* selector,
               void* arg,
               RequestCallback* requestCallback,
               long timeout) override;

 public:  // DefaultMQProducerConfig
  bool send_latency_fault_enable() const override;
  void set_send_latency_fault_enable(bool sendLatencyFaultEnable) override;

 public:
  void setRPCHook(RPCHookPtr rpcHook);

 protected:
  std::shared_ptr<MQProducer> producer_impl_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_DEFAULTMQPRODUCER_H_
