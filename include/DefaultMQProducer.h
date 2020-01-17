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

#include "MQClientConfig.h"
#include "MQProducer.h"
#include "RPCHook.h"

namespace rocketmq {

class ROCKETMQCLIENT_API DefaultMQProducerConfig : public MQClientConfig {
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
};

class ROCKETMQCLIENT_API DefaultMQProducer : public MQProducer, public DefaultMQProducerConfig {
 public:
  DefaultMQProducer(const std::string& groupname);
  DefaultMQProducer(const std::string& groupname, RPCHookPtr rpcHook);
  virtual ~DefaultMQProducer();

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

 public:  // DefaultMQProducerConfig
  bool isSendLatencyFaultEnable() const override;
  void setSendLatencyFaultEnable(bool sendLatencyFaultEnable) override;

 public:
  void setRPCHook(std::shared_ptr<RPCHook> rpcHook);

 protected:
  std::shared_ptr<MQProducer> m_producerDelegate;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PRODUCER_H__
