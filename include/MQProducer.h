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
#ifndef __MQ_PRODUCER_H__
#define __MQ_PRODUCER_H__

#include "AsyncCallback.h"
#include "MQAdmin.h"
#include "MQSelector.h"
#include "SendResult.h"
#include "TransactionListener.h"

namespace rocketmq {

class CheckTransactionStateRequestHeader;

/**
 * MQ Producer API
 */
class ROCKETMQCLIENT_API MQProducer : virtual public MQAdmin {
 public:
  virtual ~MQProducer() = default;

 public:  // MQProducer
  // Sync
  virtual SendResult send(MQMessagePtr msg) = 0;
  virtual SendResult send(MQMessagePtr msg, long timeout) = 0;
  virtual SendResult send(MQMessagePtr msg, const MQMessageQueue& mq) = 0;
  virtual SendResult send(MQMessagePtr msg, const MQMessageQueue& mq, long timeout) = 0;

  // Async
  // TODO: replace MQMessagePtr by MQMessagePtr2(shared_ptr)
  virtual void send(MQMessagePtr msg, SendCallback* sendCallback) noexcept = 0;
  virtual void send(MQMessagePtr msg, SendCallback* sendCallback, long timeout) noexcept = 0;
  virtual void send(MQMessagePtr msg, const MQMessageQueue& mq, SendCallback* sendCallback) noexcept = 0;
  virtual void send(MQMessagePtr msg, const MQMessageQueue& mq, SendCallback* sendCallback, long timeout) noexcept = 0;

  // Oneyway
  virtual void sendOneway(MQMessagePtr msg) = 0;
  virtual void sendOneway(MQMessagePtr msg, const MQMessageQueue& mq) = 0;

  // Select
  virtual SendResult send(MQMessagePtr msg, MessageQueueSelector* selector, void* arg) = 0;
  virtual SendResult send(MQMessagePtr msg, MessageQueueSelector* selector, void* arg, long timeout) = 0;
  virtual void send(MQMessagePtr msg,
                    MessageQueueSelector* selector,
                    void* arg,
                    SendCallback* sendCallback) noexcept = 0;
  virtual void send(MQMessagePtr msg,
                    MessageQueueSelector* selector,
                    void* arg,
                    SendCallback* sendCallback,
                    long timeout) noexcept = 0;
  virtual void sendOneway(MQMessagePtr msg, MessageQueueSelector* selector, void* arg) = 0;

  // Transaction
  virtual TransactionSendResult sendMessageInTransaction(MQMessagePtr msg, void* arg) = 0;

  // Batch
  virtual SendResult send(std::vector<MQMessagePtr>& msgs) = 0;
  virtual SendResult send(std::vector<MQMessagePtr>& msgs, long timeout) = 0;
  virtual SendResult send(std::vector<MQMessagePtr>& msgs, const MQMessageQueue& mq) = 0;
  virtual SendResult send(std::vector<MQMessagePtr>& msgs, const MQMessageQueue& mq, long timeout) = 0;

 public:  // MQProducerInner
  virtual TransactionListener* getCheckListener() = 0;

  virtual void checkTransactionState(const std::string& addr,
                                     MQMessageExtPtr2 msg,
                                     CheckTransactionStateRequestHeader* checkRequestHeader) = 0;

  //  virtual std::vector<std::string> getPublishTopicList() = 0;
  //  virtual void updateTopicPublishInfo(const std::string& topic, TopicPublishInfoPtr info) = 0;
};

}  // namespace rocketmq

#endif  // __MQ_PRODUCER_H__
