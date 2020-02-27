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

#ifndef __DEFAULTMQPUSHCONSUMER_H__
#define __DEFAULTMQPUSHCONSUMER_H__

#include <string>
#include "AsyncCallback.h"
#include "ConsumeType.h"
#include "MQClient.h"
#include "MQMessageListener.h"
#include "MQMessageQueue.h"
#include "SessionCredentials.h"

namespace rocketmq {
class DefaultMQPushConsumerImpl;
class ROCKETMQCLIENT_API DefaultMQPushConsumer {
 public:
  DefaultMQPushConsumer(const std::string& groupname);

  virtual ~DefaultMQPushConsumer();

  virtual void start();
  virtual void shutdown();
  virtual std::string version();

  const std::string& getNamesrvAddr() const;
  void setNamesrvAddr(const std::string& namesrvAddr);

  void setSessionCredentials(const std::string& accessKey,
                             const std::string& secretKey,
                             const std::string& accessChannel);
  const SessionCredentials& getSessionCredentials() const;

  void subscribe(const std::string& topic, const std::string& subExpression);

  void registerMessageListener(MQMessageListener* pMessageListener);
  MessageListenerType getMessageListenerType();

  MessageModel getMessageModel() const;
  void setMessageModel(MessageModel messageModel);

  void setConsumeFromWhere(ConsumeFromWhere consumeFromWhere);
  ConsumeFromWhere getConsumeFromWhere();

  const std::string& getNamesrvDomain() const;
  void setNamesrvDomain(const std::string& namesrvDomain);

  const std::string& getInstanceName() const;
  void setInstanceName(const std::string& instanceName);

  const std::string& getNameSpace() const;
  void setNameSpace(const std::string& nameSpace);

  const std::string& getGroupName() const;
  void setGroupName(const std::string& groupname);

  /**
   * Log configuration interface, default LOG_LEVEL is LOG_LEVEL_INFO, default
   * log file num is 3, each log size is 100M
   **/
  void setLogLevel(elogLevel inputLevel);
  elogLevel getLogLevel();
  void setLogPath(const std::string& logPath);
  void setLogFileSizeAndNum(int fileNum, long perFileSize);  // perFileSize is MB unit

  void setConsumeThreadCount(int threadCount);
  int getConsumeThreadCount() const;

  void setMaxReconsumeTimes(int maxReconsumeTimes);
  int getMaxReconsumeTimes() const;

  void setPullMsgThreadPoolCount(int threadCount);
  int getPullMsgThreadPoolCount() const;

  void setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize);
  int getConsumeMessageBatchMaxSize() const;

  /**
   * Set max cache msg size perQueue in memory if consumer could not consume msgs
   * immediately
   * default maxCacheMsgSize perQueue is 1000, set range is:1~65535
   **/
  void setMaxCacheMsgSizePerQueue(int maxCacheSize);
  int getMaxCacheMsgSizePerQueue() const;

  /** Set TcpTransport pull thread num, which dermine the num of threads to
   *  distribute network data,
   *  1. its default value is CPU num, it must be setted before producer/consumer
   *     start, minimum value is CPU num;
   *  2. this pullThread num must be tested on your environment to find the best
   *     value for RT of sendMsg or delay time of consume msg before you change it;
   *  3. producer and consumer need different pullThread num, if set this num,
   *     producer and consumer must set different instanceName.
   **/
  void setTcpTransportPullThreadNum(int num);
  int getTcpTransportPullThreadNum() const;

  /** Timeout of tcp connect, it is same meaning for both producer and consumer;
   *    1. default value is 3000ms
   *    2. input parameter could only be milliSecond, suggestion value is
   *       1000-3000ms;
   **/
  void setTcpTransportConnectTimeout(uint64_t timeout);  // ms
  uint64_t getTcpTransportConnectTimeout() const;

  /** Timeout of tryLock tcpTransport before sendMsg/pullMsg, if timeout,
   *  returns NULL
   *    1. paremeter unit is ms, default value is 3000ms, the minimun value is 1000ms
   *       suggestion value is 3000ms;
   *    2. if configured with value smaller than 1000ms, the tryLockTimeout value
   *       will be setted to 1000ms
   **/
  void setTcpTransportTryLockTimeout(uint64_t timeout);  // ms
  uint64_t getTcpTransportTryLockTimeout() const;

  void setUnitName(std::string unitName);
  const std::string& getUnitName() const;

  void setAsyncPull(bool asyncFlag);

 private:
  DefaultMQPushConsumerImpl* impl;
};
}  // namespace rocketmq
#endif
