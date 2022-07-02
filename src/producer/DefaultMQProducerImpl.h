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

#ifndef __DEFAULTMQPRODUCERIMPL_H__
#define __DEFAULTMQPRODUCERIMPL_H__

#include "BatchMessage.h"
#include "MQMessageQueue.h"
#include "MQProducer.h"
#include "RocketMQClient.h"
#include "SendMessageContext.h"
#include "SendMessageHook.h"
#include "SendResult.h"

namespace rocketmq {
//<!***************************************************************************
class DefaultMQProducerImpl : public MQProducer {
 public:
  DefaultMQProducerImpl(const std::string& groupname);
  virtual ~DefaultMQProducerImpl();

  //<!begin mqadmin;
  virtual void start();
  virtual void shutdown();
  virtual void start(bool factoryStart);
  virtual void shutdown(bool factoryStart);
  //<!end mqadmin;

  //<! begin MQProducer;
  virtual SendResult send(MQMessage& msg, bool bSelectActiveBroker = false);
  virtual SendResult send(MQMessage& msg, const MQMessageQueue& mq);
  virtual SendResult send(MQMessage& msg, MessageQueueSelector* selector, void* arg);
  virtual SendResult send(MQMessage& msg,
                          MessageQueueSelector* selector,
                          void* arg,
                          int autoRetryTimes,
                          bool bActiveBroker = false);
  virtual SendResult send(std::vector<MQMessage>& msgs);
  virtual SendResult send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq);
  virtual void send(MQMessage& msg, SendCallback* pSendCallback, bool bSelectActiveBroker = false);
  virtual void send(MQMessage& msg, const MQMessageQueue& mq, SendCallback* pSendCallback);
  virtual void send(MQMessage& msg, MessageQueueSelector* selector, void* arg, SendCallback* pSendCallback);
  virtual void sendOneway(MQMessage& msg, bool bSelectActiveBroker = false);
  virtual void sendOneway(MQMessage& msg, const MQMessageQueue& mq);
  virtual void sendOneway(MQMessage& msg, MessageQueueSelector* selector, void* arg);
  //<! end MQProducer;

  // set and get timeout of per msg
  int getSendMsgTimeout() const;
  void setSendMsgTimeout(int sendMsgTimeout);

  /*
  *  if msgBody size is large than m_compressMsgBodyOverHowmuch
      rocketmq cpp will compress msgBody according to compressLevel
  */
  int getCompressMsgBodyOverHowmuch() const;
  void setCompressMsgBodyOverHowmuch(int compressMsgBodyOverHowmuch);
  int getCompressLevel() const;
  void setCompressLevel(int compressLevel);

  // if msgbody size larger than maxMsgBodySize, exception will be throwed
  int getMaxMessageSize() const;
  void setMaxMessageSize(int maxMessageSize);

  // set msg max retry times, default retry times is 5
  int getRetryTimes() const;
  void setRetryTimes(int times);

  int getRetryTimes4Async() const;
  void setRetryTimes4Async(int times);
  void submitSendTraceRequest(const MQMessage& msg, SendCallback* pSendCallback);

 protected:
  SendResult sendAutoRetrySelectImpl(MQMessage& msg,
                                     MessageQueueSelector* pSelector,
                                     void* pArg,
                                     int communicationMode,
                                     SendCallback* pSendCallback,
                                     int retryTimes,
                                     bool bActiveBroker = false);
  SendResult sendSelectImpl(MQMessage& msg,
                            MessageQueueSelector* pSelector,
                            void* pArg,
                            int communicationMode,
                            SendCallback* sendCallback);
  SendResult sendDefaultImpl(MQMessage& msg,
                             int communicationMode,
                             SendCallback* pSendCallback,
                             bool bActiveBroker = false);
  SendResult sendKernelImpl(MQMessage& msg,
                            const MQMessageQueue& mq,
                            int communicationMode,
                            SendCallback* pSendCallback);
  bool tryToCompressMessage(MQMessage& msg);
  BatchMessage buildBatchMessage(std::vector<MQMessage>& msgs);
  bool dealWithNameSpace();
  void logConfigs();
  bool dealWithMessageTrace();
  bool isMessageTraceTopic(const std::string& topic);
  bool hasSendMessageHook();
  void registerSendMessageHook(std::shared_ptr<SendMessageHook>& hook);
  void executeSendMessageHookBefore(SendMessageContext* context);
  void executeSendMessageHookAfter(SendMessageContext* context);

  void sendTraceMessage(MQMessage& msg, SendCallback* pSendCallback);

 private:
  int m_sendMsgTimeout;
  int m_compressMsgBodyOverHowmuch;
  int m_maxMessageSize;  //<! default:128K;
  // bool m_retryAnotherBrokerWhenNotStoreOK;
  int m_compressLevel;
  int m_retryTimes;
  int m_retryTimes4Async;

  // used for trace
  std::vector<std::shared_ptr<SendMessageHook> > m_sendMessageHookList;
  boost::asio::io_service m_trace_ioService;
  boost::thread_group m_trace_threadpool;
  boost::asio::io_service::work m_trace_ioService_work;
};
//<!***************************************************************************
}  // namespace rocketmq
#endif
