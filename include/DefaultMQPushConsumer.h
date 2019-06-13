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

#ifndef __DEFAULT_MQ_PUSH_CONSUMER_H__
#define __DEFAULT_MQ_PUSH_CONSUMER_H__

#include <string>

#include "concurrent/executor.hpp"

#include "AsyncCallback.h"
#include "MQConsumer.h"
#include "MQMessageListener.h"
#include "MQMessageQueue.h"

namespace rocketmq {

class Rebalance;
class SubscriptionData;
class OffsetStore;
class PullAPIWrapper;
class PullRequest;
class ConsumeMsgService;
class TaskQueue;
class TaskThread;
class AsyncPullCallback;
class ConsumerRunningInfo;

class ROCKETMQCLIENT_API DefaultMQPushConsumer : public MQConsumer {
 public:
  DefaultMQPushConsumer(const std::string& groupname);
  ~DefaultMQPushConsumer() override;

  //<!begin mqadmin;
  void start() override;
  void shutdown() override;
  //<!end mqadmin;

  //<!begin MQConsumer
  void sendMessageBack(MQMessageExt& msg, int delayLevel) override;
  void fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) override;
  void doRebalance() override;
  void persistConsumerOffset() override;
  void persistConsumerOffsetByResetOffset() override;
  void updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info) override;
  ConsumeType getConsumeType() override;
  ConsumeFromWhere getConsumeFromWhere() override;
  void setConsumeFromWhere(ConsumeFromWhere consumeFromWhere);
  void getSubscriptions(std::vector<SubscriptionData>&) override;
  void updateConsumeOffset(const MQMessageQueue& mq, int64 offset) override;
  void removeConsumeOffset(const MQMessageQueue& mq) override;
  PullResult pull(const MQMessageQueue& mq, const std::string& subExpression, int64 offset, int maxNums) override {
    return PullResult();
  }
  void pull(const MQMessageQueue& mq,
            const std::string& subExpression,
            int64 offset,
            int maxNums,
            PullCallback* pPullCallback) override {}
  ConsumerRunningInfo* getConsumerRunningInfo() override;
  //<!end MQConsumer;

  void registerMessageListener(MQMessageListener* pMessageListener);
  MessageListenerType getMessageListenerType();
  void subscribe(const std::string& topic, const std::string& subExpression);

  OffsetStore* getOffsetStore() const;
  Rebalance* getRebalance() const override;
  ConsumeMsgService* getConsumerMsgService() const;

  void producePullMsgTask(PullRequest*) override;
  void runPullMsgQueue(TaskQueue* pTaskQueue);
  void pullMessage(PullRequest* pullrequest);       // sync pullMsg
  void pullMessageAsync(PullRequest* pullrequest);  // async pullMsg
  void setAsyncPull(bool asyncFlag);
  AsyncPullCallback* getAsyncPullCallBack(PullRequest* request, MQMessageQueue msgQueue);
  void shutdownAsyncPullCallBack();

  /*
    for orderly consume, set the pull num of message size by each pullMsg,
    default value is 1;
  */
  void setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize);
  int getConsumeMessageBatchMaxSize() const;

  /*
    set consuming thread count, default value is cpu cores
  */
  void setConsumeThreadCount(int threadCount);
  int getConsumeThreadCount() const;

  /*
    set pullMsg thread count, default value is cpu cores
  */
  void setPullMsgThreadPoolCount(int threadCount);
  int getPullMsgThreadPoolCount() const;

  /*
    set max cache msg size perQueue in memory if consumer could not consume msgs
    immediately
    default maxCacheMsgSize perQueue is 1000, set range is:1~65535
  */
  void setMaxCacheMsgSizePerQueue(int maxCacheSize);
  int getMaxCacheMsgSizePerQueue() const;

 private:
  void checkConfig();
  void copySubscription();
  void updateTopicSubscribeInfoWhenSubscriptionChanged();

 private:
  uint64_t m_startTime;
  ConsumeFromWhere m_consumeFromWhere;
  std::map<std::string, std::string> m_subTopics;
  int m_consumeThreadCount;
  OffsetStore* m_pOffsetStore;
  Rebalance* m_pRebalance;
  PullAPIWrapper* m_pPullAPIWrapper;
  ConsumeMsgService* m_consumerService;
  MQMessageListener* m_pMessageListener;
  int m_consumeMessageBatchMaxSize;
  int m_maxMsgCacheSize;

  typedef std::map<MQMessageQueue, AsyncPullCallback*> PullMAP;
  PullMAP m_PullCallback;
  bool m_asyncPull;
  int m_asyncPullTimeout;
  int m_pullMsgThreadPoolNum;

 private:
  scheduled_thread_pool_executor* m_pullMessageService;
};

}  // namespace rocketmq

#endif  // __DEFAULT_MQ_PUSH_CONSUMER_H__
