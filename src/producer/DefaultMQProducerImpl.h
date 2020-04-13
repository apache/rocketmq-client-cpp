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
#ifndef __DEFAULT_MQ_PRODUCER_IMPL_H__
#define __DEFAULT_MQ_PRODUCER_IMPL_H__

#include "CommunicationMode.h"
#include "DefaultMQProducer.h"
#include "MQClientImpl.h"
#include "MQProducerInner.h"
#include "MessageBatch.h"

namespace rocketmq {

class TopicPublishInfo;
class MQFaultStrategy;
class thread_pool_executor;

class DefaultMQProducerImpl;
typedef std::shared_ptr<DefaultMQProducerImpl> DefaultMQProducerImplPtr;

class DefaultMQProducerImpl : public std::enable_shared_from_this<DefaultMQProducerImpl>,
                              public MQProducer,
                              public MQClientImpl,
                              public MQProducerInner {
 public:
  static DefaultMQProducerImplPtr create(DefaultMQProducerConfigPtr config, RPCHookPtr rpcHook = nullptr) {
    if (nullptr == rpcHook) {
      return DefaultMQProducerImplPtr(new DefaultMQProducerImpl(config));
    } else {
      return DefaultMQProducerImplPtr(new DefaultMQProducerImpl(config, rpcHook));
    }
  }

 private:
  DefaultMQProducerImpl(DefaultMQProducerConfigPtr config);
  DefaultMQProducerImpl(DefaultMQProducerConfigPtr config, RPCHookPtr rpcHook);

 public:
  virtual ~DefaultMQProducerImpl();

 public:  // MQProducer
  void start() override;
  void shutdown() override;

  // Sync: caller will be responsible for the lifecycle of messages.
  SendResult send(MQMessagePtr msg) override;
  SendResult send(MQMessagePtr msg, long timeout) override;
  SendResult send(MQMessagePtr msg, const MQMessageQueue& mq) override;
  SendResult send(MQMessagePtr msg, const MQMessageQueue& mq, long timeout) override;

  // Async: don't delete msg object, until callback occur.
  void send(MQMessagePtr msg, SendCallback* sendCallback) noexcept override;
  void send(MQMessagePtr msg, SendCallback* sendCallback, long timeout) noexcept override;
  void send(MQMessagePtr msg, const MQMessageQueue& mq, SendCallback* sendCallback) noexcept override;
  void send(MQMessagePtr msg, const MQMessageQueue& mq, SendCallback* sendCallback, long timeout) noexcept override;

  // Oneyway: same as sync send, but don't care its result.
  void sendOneway(MQMessagePtr msg) override;
  void sendOneway(MQMessagePtr msg, const MQMessageQueue& mq) override;

  // Select
  SendResult send(MQMessagePtr msg, MessageQueueSelector* selector, void* arg) override;
  SendResult send(MQMessagePtr msg, MessageQueueSelector* selector, void* arg, long timeout) override;
  void send(MQMessagePtr msg, MessageQueueSelector* selector, void* arg, SendCallback* sendCallback) noexcept override;
  void send(MQMessagePtr msg,
            MessageQueueSelector* selector,
            void* arg,
            SendCallback* sendCallback,
            long timeout) noexcept override;
  void sendOneway(MQMessagePtr msg, MessageQueueSelector* selector, void* arg) override;

  // Transaction
  TransactionSendResult sendMessageInTransaction(MQMessagePtr msg, void* arg) override;

  // Batch: power by sync send, caller will be responsible for the lifecycle of messages.
  SendResult send(std::vector<MQMessagePtr>& msgs) override;
  SendResult send(std::vector<MQMessagePtr>& msgs, long timeout) override;
  SendResult send(std::vector<MQMessagePtr>& msgs, const MQMessageQueue& mq) override;
  SendResult send(std::vector<MQMessagePtr>& msgs, const MQMessageQueue& mq, long timeout) override;

 public:  // MQProducerInner
  TransactionListener* getCheckListener() override;

  void checkTransactionState(const std::string& addr,
                             MQMessageExtPtr2 msg,
                             CheckTransactionStateRequestHeader* checkRequestHeader) override;

 public:
  void initTransactionEnv();
  void destroyTransactionEnv();

  const MQMessageQueue& selectOneMessageQueue(TopicPublishInfo* tpInfo, const std::string& lastBrokerName);
  void updateFaultItem(const std::string& brokerName, const long currentLatency, bool isolation);

  void endTransaction(SendResult& sendResult,
                      LocalTransactionState localTransactionState,
                      std::exception_ptr& localException);

  bool isSendLatencyFaultEnable() const;
  void setSendLatencyFaultEnable(bool sendLatencyFaultEnable);

 protected:
  SendResult* sendDefaultImpl(MQMessagePtr msg,
                              CommunicationMode communicationMode,
                              SendCallback* sendCallback,
                              long timeout);
  SendResult* sendKernelImpl(MQMessagePtr msg,
                             const MQMessageQueue& mq,
                             CommunicationMode communicationMode,
                             SendCallback* sendCallback,
                             std::shared_ptr<TopicPublishInfo> topicPublishInfo,
                             long timeout);
  SendResult* sendSelectImpl(MQMessagePtr msg,
                             MessageQueueSelector* selector,
                             void* arg,
                             CommunicationMode communicationMode,
                             SendCallback* sendCallback,
                             long timeout);

  TransactionSendResult* sendMessageInTransactionImpl(MQMessagePtr msg, void* arg, long timeout);
  void checkTransactionStateImpl(const std::string& addr,
                                 MQMessageExtPtr2 message,
                                 long tranStateTableOffset,
                                 long commitLogOffset,
                                 const std::string& msgId,
                                 const std::string& transactionId,
                                 const std::string& offsetMsgId);

  bool tryToCompressMessage(MQMessage& msg);

  MessageBatch* batch(std::vector<MQMessagePtr>& msgs);

 private:
  DefaultMQProducerConfigPtr m_producerConfig;
  std::unique_ptr<MQFaultStrategy> m_mqFaultStrategy;
  std::unique_ptr<thread_pool_executor> m_checkTransactionExecutor;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PRODUCER_IMPL_H__
