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
#ifndef ROCKETMQ_MQPRODUCER_H_
#define ROCKETMQ_MQPRODUCER_H_

#include "MQMessageQueue.h"
#include "MQSelector.h"
#include "RequestCallback.h"
#include "SendCallback.h"
#include "SendResult.h"
#include "TransactionSendResult.h"

namespace rocketmq {

/**
 * MQProducer - interface for producer
 */
class ROCKETMQCLIENT_API MQProducer {
 public:
  virtual ~MQProducer() = default;

 public:  // MQProducer
  virtual void start() = 0;
  virtual void shutdown() = 0;

  virtual std::vector<MQMessageQueue> fetchPublishMessageQueues(const std::string& topic) = 0;

  // Sync
  virtual SendResult send(MQMessage& msg) = 0;
  virtual SendResult send(MQMessage& msg, long timeout) = 0;
  virtual SendResult send(MQMessage& msg, const MQMessageQueue& mq) = 0;
  virtual SendResult send(MQMessage& msg, const MQMessageQueue& mq, long timeout) = 0;

  // Async
  virtual void send(MQMessage& msg, SendCallback* sendCallback) noexcept = 0;
  virtual void send(MQMessage& msg, SendCallback* sendCallback, long timeout) noexcept = 0;
  virtual void send(MQMessage& msg, const MQMessageQueue& mq, SendCallback* sendCallback) noexcept = 0;
  virtual void send(MQMessage& msg, const MQMessageQueue& mq, SendCallback* sendCallback, long timeout) noexcept = 0;

  // Oneyway
  virtual void sendOneway(MQMessage& msg) = 0;
  virtual void sendOneway(MQMessage& msg, const MQMessageQueue& mq) = 0;

  // Select
  virtual SendResult send(MQMessage& msg, MessageQueueSelector* selector, void* arg) = 0;
  virtual SendResult send(MQMessage& msg, MessageQueueSelector* selector, void* arg, long timeout) = 0;
  virtual void send(MQMessage& msg, MessageQueueSelector* selector, void* arg, SendCallback* sendCallback) noexcept = 0;
  virtual void send(MQMessage& msg,
                    MessageQueueSelector* selector,
                    void* arg,
                    SendCallback* sendCallback,
                    long timeout) noexcept = 0;
  virtual void sendOneway(MQMessage& msg, MessageQueueSelector* selector, void* arg) = 0;

  // Transaction
  virtual TransactionSendResult sendMessageInTransaction(MQMessage& msg, void* arg) = 0;

  // Batch
  virtual SendResult send(std::vector<MQMessage>& msgs) = 0;
  virtual SendResult send(std::vector<MQMessage>& msgs, long timeout) = 0;
  virtual SendResult send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq) = 0;
  virtual SendResult send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq, long timeout) = 0;

  virtual void send(std::vector<MQMessage>& msgs, SendCallback* sendCallback) = 0;
  virtual void send(std::vector<MQMessage>& msgs, SendCallback* sendCallback, long timeout) = 0;
  virtual void send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq, SendCallback* sendCallback) = 0;
  virtual void send(std::vector<MQMessage>& msgs,
                    const MQMessageQueue& mq,
                    SendCallback* sendCallback,
                    long timeout) = 0;

  // RPC
  virtual MQMessage request(MQMessage& msg, long timeout) = 0;
  virtual void request(MQMessage& msg, RequestCallback* requestCallback, long timeout) = 0;
  virtual MQMessage request(MQMessage& msg, const MQMessageQueue& mq, long timeout) = 0;
  virtual void request(MQMessage& msg, const MQMessageQueue& mq, RequestCallback* requestCallback, long timeout) = 0;
  virtual MQMessage request(MQMessage& msg, MessageQueueSelector* selector, void* arg, long timeout) = 0;
  virtual void request(MQMessage& msg,
                       MessageQueueSelector* selector,
                       void* arg,
                       RequestCallback* requestCallback,
                       long timeout) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQPRODUCER_H_
