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
#ifndef __DEFAULT_MQ_PRODUCER_H__
#define __DEFAULT_MQ_PRODUCER_H__

#include "CommunicationMode.h"
#include "MQClient.h"
#include "MQProducer.h"
#include "MessageBatch.h"
//#include "TopicPublishInfo.h"

namespace rocketmq {

class TopicPublishInfo;
class MQFaultStrategy;
class thread_pool_executor;

class ROCKETMQCLIENT_API DefaultMQProducerConfig {
 public:
  DefaultMQProducerConfig();
  virtual ~DefaultMQProducerConfig() = default;

  // if msgbody size larger than maxMsgBodySize, exception will be throwed
  int getMaxMessageSize() const { return m_maxMessageSize; }
  void setMaxMessageSize(int maxMessageSize) { m_maxMessageSize = maxMessageSize; }

  /*
   * if msgBody size is large than m_compressMsgBodyOverHowmuch
   *  rocketmq cpp will compress msgBody according to compressLevel
   */
  int getCompressMsgBodyOverHowmuch() const { return m_compressMsgBodyOverHowmuch; }
  void setCompressMsgBodyOverHowmuch(int compressMsgBodyOverHowmuch) {
    m_compressMsgBodyOverHowmuch = compressMsgBodyOverHowmuch;
  }

  int getCompressLevel() const { return m_compressLevel; }
  void setCompressLevel(int compressLevel) {
    if ((compressLevel >= 0 && compressLevel <= 9) || compressLevel == -1) {
      m_compressLevel = compressLevel;
    }
  }

  // set and get timeout of per msg
  int getSendMsgTimeout() const { return m_sendMsgTimeout; }
  void setSendMsgTimeout(int sendMsgTimeout) { m_sendMsgTimeout = sendMsgTimeout; }

  // set msg max retry times, default retry times is 5
  int getRetryTimes() const { return m_retryTimes; }
  void setRetryTimes(int times) { m_retryTimes = std::min(std::max(0, times), 15); }

  int getRetryTimes4Async() const { return m_retryTimes4Async; }
  void setRetryTimes4Async(int times) { m_retryTimes4Async = std::min(std::max(0, times), 15); }

  bool isRetryAnotherBrokerWhenNotStoreOK() const { return m_retryAnotherBrokerWhenNotStoreOK; }
  void setRetryAnotherBrokerWhenNotStoreOK(bool retryAnotherBrokerWhenNotStoreOK) {
    m_retryAnotherBrokerWhenNotStoreOK = retryAnotherBrokerWhenNotStoreOK;
  }

  bool isSendMessageInTransactionEnable() const { return m_sendMessageInTransactionEnable; }
  void setSendMessageInTransactionEnable(bool sendMessageInTransactionEnable) {
    m_sendMessageInTransactionEnable = sendMessageInTransactionEnable;
  }

  TransactionListener* getTransactionListener() const { return m_transactionListener; }
  void setTransactionListener(TransactionListener* transactionListener) { m_transactionListener = transactionListener; }

  virtual bool isSendLatencyFaultEnable() const = 0;
  virtual void setSendLatencyFaultEnable(bool sendLatencyFaultEnable) = 0;

 protected:
  int m_maxMessageSize;              // default: 4 MB
  int m_compressMsgBodyOverHowmuch;  // default: 4 KB
  int m_compressLevel;
  int m_sendMsgTimeout;
  int m_retryTimes;
  int m_retryTimes4Async;
  bool m_retryAnotherBrokerWhenNotStoreOK;

  // transcations
  bool m_sendMessageInTransactionEnable;
  TransactionListener* m_transactionListener;
  std::unique_ptr<thread_pool_executor> m_checkTransactionExecutor;
};

class DefaultMQProducer;
typedef std::shared_ptr<DefaultMQProducer> DefaultMQProducerPtr;

class ROCKETMQCLIENT_API DefaultMQProducer : public std::enable_shared_from_this<DefaultMQProducer>,
                                             public MQProducer,
                                             public MQClient,
                                             public DefaultMQProducerConfig {
 public:
  static DefaultMQProducerPtr create(const std::string& groupname = "", RPCHookPtr rpcHook = nullptr) {
    if (nullptr == rpcHook) {
      return DefaultMQProducerPtr(new DefaultMQProducer(groupname));
    } else {
      return DefaultMQProducerPtr(new DefaultMQProducer(groupname, rpcHook));
    }
  }

 private:
  DefaultMQProducer(const std::string& groupname);
  DefaultMQProducer(const std::string& groupname, RPCHookPtr rpcHook);

 public:
  virtual ~DefaultMQProducer();

 public:  // MQClient
  void start() override;
  void shutdown() override;

 public:  // MQProducer
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
  TransactionListener* getCheckListener() override { return getTransactionListener(); };

  void checkTransactionState(const std::string& addr,
                             MQMessageExtPtr2 msg,
                             CheckTransactionStateRequestHeader* checkRequestHeader) override;

 public:
  const MQMessageQueue& selectOneMessageQueue(TopicPublishInfo* tpInfo, const std::string& lastBrokerName);
  void updateFaultItem(const std::string& brokerName, const long currentLatency, bool isolation);

  void endTransaction(SendResult& sendResult,
                      LocalTransactionState localTransactionState,
                      std::exception_ptr& localException);

  bool isSendLatencyFaultEnable() const override;
  void setSendLatencyFaultEnable(bool sendLatencyFaultEnable) override;

 protected:
  void initTransactionEnv();
  void destroyTransactionEnv();

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
  std::unique_ptr<MQFaultStrategy> m_mqFaultStrategy;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PRODUCER_H__
