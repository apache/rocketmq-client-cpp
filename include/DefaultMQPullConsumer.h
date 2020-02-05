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

#ifndef __DEFAULTMQPULLCONSUMER_H__
#define __DEFAULTMQPULLCONSUMER_H__

#include <set>
#include <string>
#include "AsyncCallback.h"
#include "ConsumeType.h"
#include "MQClient.h"
#include "MQMessage.h"
#include "MQMessageExt.h"
#include "MQMessageQueue.h"
#include "MQueueListener.h"
#include "PullResult.h"
#include "RocketMQClient.h"

namespace rocketmq {
class SubscriptionData;
class ConsumerRunningInfo;
//<!***************************************************************************
class ROCKETMQCLIENT_API DefaultMQPullConsumer {
 public:
  DefaultMQPullConsumer(const std::string& groupname);
  virtual ~DefaultMQPullConsumer();

  //<!begin mqadmin;
  virtual void start();
  virtual void shutdown();
  //<!end mqadmin;
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

  void setSessionCredentials(const std::string& accessKey,
                             const std::string& secretKey,
                             const std::string& accessChannel);
  //<!begin MQConsumer
  virtual void fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs);
  virtual void doRebalance();
  virtual void persistConsumerOffset();
  virtual void persistConsumerOffsetByResetOffset();
  virtual void updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info);
  virtual ConsumeType getConsumeType();
  virtual ConsumeFromWhere getConsumeFromWhere();
  virtual void getSubscriptions(std::vector<SubscriptionData>&);
  virtual void updateConsumeOffset(const MQMessageQueue& mq, int64 offset);
  virtual void removeConsumeOffset(const MQMessageQueue& mq);
  //<!end MQConsumer;

  void registerMessageQueueListener(const std::string& topic, MQueueListener* pListener);
  /**
   * Pull message from specified queue, if no msg in queue, return directly
   *
   * @param mq
   *            specify the pulled queue
   * @param subExpression
   *            set filter expression for pulled msg, broker will filter msg actively
   *            Now only OR operation is supported, eg: "tag1 || tag2 || tag3"
   *            if subExpression is setted to "null" or "*", all msg will be subscribed
   * @param offset
   *            specify the started pull offset
   * @param maxNums
   *            specify max msg num by per pull
   * @return
   *            PullResult
   */
  virtual PullResult pull(const MQMessageQueue& mq, const std::string& subExpression, int64 offset, int maxNums);
  virtual void pull(const MQMessageQueue& mq,
                    const std::string& subExpression,
                    int64 offset,
                    int maxNums,
                    PullCallback* pPullCallback);

  /**
   * pull msg from specified queue, if no msg, broker will suspend the pull request 20s
   *
   * @param mq
   *            specify the pulled queue
   * @param subExpression
   *            set filter expression for pulled msg, broker will filter msg actively
   *            Now only OR operation is supported, eg: "tag1 || tag2 || tag3"
   *            if subExpression is setted to "null" or "*", all msg will be subscribed
   * @param offset
   *            specify the started pull offset
   * @param maxNums
   *            specify max msg num by per pull
   * @return
   *            accroding to PullResult
   */
  PullResult pullBlockIfNotFound(const MQMessageQueue& mq, const std::string& subExpression, int64 offset, int maxNums);
  void pullBlockIfNotFound(const MQMessageQueue& mq,
                           const std::string& subExpression,
                           int64 offset,
                           int maxNums,
                           PullCallback* pPullCallback);

  virtual ConsumerRunningInfo* getConsumerRunningInfo() { return NULL; }

  int64 fetchConsumeOffset(const MQMessageQueue& mq, bool fromStore);

  void fetchMessageQueuesInBalance(const std::string& topic, std::vector<MQMessageQueue> mqs);

  // temp persist consumer offset interface, only valid with
  // RemoteBrokerOffsetStore, updateConsumeOffset should be called before.
  void persistConsumerOffset4PullConsumer(const MQMessageQueue& mq);
};
//<!***************************************************************************
}  // namespace rocketmq
#endif
