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

#ifndef __DEFAULTMQPRODUCER_H__
#define __DEFAULTMQPRODUCER_H__

#include "AsyncCallback.h"
#include "MQClient.h"
#include "MQMessageQueue.h"
#include "MQSelector.h"
#include "RocketMQClient.h"
#include "SendResult.h"
#include "SessionCredentials.h"

namespace rocketmq {
class DefaultMQProducerImpl;
//<!***************************************************************************
class ROCKETMQCLIENT_API DefaultMQProducer {
 public:
  DefaultMQProducer(const std::string& groupname);
  virtual ~DefaultMQProducer();

  virtual void start();
  virtual void shutdown();

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

  int getSendMsgTimeout() const;
  void setSendMsgTimeout(int sendMsgTimeout);

  /*
   *  if msgBody size is large than m_compressMsgBodyOverHowmuch
   *  rocketmq cpp will compress msgBody according to compressLevel
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
  const std::string& getNamesrvAddr() const;
  void setNamesrvAddr(const std::string& namesrvAddr);
  const std::string& getNamesrvDomain() const;
  void setNamesrvDomain(const std::string& namesrvDomain);
  const std::string& getInstanceName() const;
  void setInstanceName(const std::string& instanceName);
  // nameSpace
  const std::string& getNameSpace() const;
  void setNameSpace(const std::string& nameSpace);
  const std::string& getGroupName() const;
  void setGroupName(const std::string& groupname);

  // log configuration interface, default LOG_LEVEL is LOG_LEVEL_INFO, default
  // log file num is 3, each log size is 100M
  void setLogLevel(elogLevel inputLevel);
  elogLevel getLogLevel();
  void setLogFileSizeAndNum(int fileNum, long perFileSize);  // perFileSize is MB unit

  /** set TcpTransport pull thread num, which dermine the num of threads to
   *  distribute network data,
   *  1. its default value is CPU num, it must be setted before producer/consumer
   *     start, minimum value is CPU num;
   *  2. this pullThread num must be tested on your environment to find the best
   *     value for RT of sendMsg or delay time of consume msg before you change it;
   *  3. producer and consumer need different pullThread num, if set this num,
   *     producer and consumer must set different instanceName.
   **/
  void setTcpTransportPullThreadNum(int num);
  const int getTcpTransportPullThreadNum() const;

  /** timeout of tcp connect, it is same meaning for both producer and consumer;
   *    1. default value is 3000ms
   *    2. input parameter could only be milliSecond, suggestion value is
   *       1000-3000ms;
   **/
  void setTcpTransportConnectTimeout(uint64_t timeout);  // ms
  const uint64_t getTcpTransportConnectTimeout() const;

  /** timeout of tryLock tcpTransport before sendMsg/pullMsg, if timeout,
   *  returns NULL
   *    1. paremeter unit is ms, default value is 3000ms, the minimun value is 1000ms
   *       suggestion value is 3000ms;
   *    2. if configured with value smaller than 1000ms, the tryLockTimeout value
   *       will be setted to 1000ms
   **/
  void setTcpTransportTryLockTimeout(uint64_t timeout);  // ms
  const uint64_t getTcpTransportTryLockTimeout() const;

  void setUnitName(std::string unitName);
  const std::string& getUnitName() const;

  void setSessionCredentials(const std::string& accessKey,
                             const std::string& secretKey,
                             const std::string& accessChannel);
  const SessionCredentials& getSessionCredentials() const;

 private:
  DefaultMQProducerImpl* impl;
};
//<!***************************************************************************
}  // namespace rocketmq
#endif
