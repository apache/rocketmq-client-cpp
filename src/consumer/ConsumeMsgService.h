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
#ifndef ROCKETMQ_CONSUMER_CONSUMEMSGSERVICE_H_
#define ROCKETMQ_CONSUMER_CONSUMEMSGSERVICE_H_

#include "DefaultMQPushConsumerImpl.h"
#include "Logging.h"
#include "MQMessageListener.h"
#include "MessageQueueLock.hpp"
#include "PullRequest.h"
#include "concurrent/executor.hpp"

namespace rocketmq {

class ConsumeMsgService {
 public:
  ConsumeMsgService() = default;
  virtual ~ConsumeMsgService() = default;

  virtual void start() {}
  virtual void shutdown() {}
  virtual void submitConsumeRequest(std::vector<MessageExtPtr>& msgs,
                                    ProcessQueuePtr processQueue,
                                    const MQMessageQueue& messageQueue,
                                    const bool dispathToConsume) = 0;
};

class ConsumeMessageConcurrentlyService : public ConsumeMsgService {
 public:
  ConsumeMessageConcurrentlyService(DefaultMQPushConsumerImpl*, int threadCount, MQMessageListener* msgListener);
  ~ConsumeMessageConcurrentlyService() override;

  void start() override;
  void shutdown() override;

  void submitConsumeRequest(std::vector<MessageExtPtr>& msgs,
                            ProcessQueuePtr processQueue,
                            const MQMessageQueue& messageQueue,
                            const bool dispathToConsume) override;

  void ConsumeRequest(std::vector<MessageExtPtr>& msgs,
                      ProcessQueuePtr processQueue,
                      const MQMessageQueue& messageQueue);

 private:
  void submitConsumeRequestLater(std::vector<MessageExtPtr>& msgs,
                                 ProcessQueuePtr processQueue,
                                 const MQMessageQueue& messageQueue);

 private:
  DefaultMQPushConsumerImpl* consumer_;
  MQMessageListener* message_listener_;

  thread_pool_executor consume_executor_;
  scheduled_thread_pool_executor scheduled_executor_service_;
};

class ConsumeMessageOrderlyService : public ConsumeMsgService {
 public:
  ConsumeMessageOrderlyService(DefaultMQPushConsumerImpl*, int threadCount, MQMessageListener* msgListener);
  ~ConsumeMessageOrderlyService() override;

  void start() override;
  void shutdown() override;
  void stopThreadPool();

  void submitConsumeRequest(std::vector<MessageExtPtr>& msgs,
                            ProcessQueuePtr processQueue,
                            const MQMessageQueue& messageQueue,
                            const bool dispathToConsume) override;
  void submitConsumeRequestLater(ProcessQueuePtr processQueue,
                                 const MQMessageQueue& messageQueue,
                                 const long suspendTimeMillis);
  void tryLockLaterAndReconsume(const MQMessageQueue& mq, ProcessQueuePtr processQueue, const long delayMills);

  void ConsumeRequest(ProcessQueuePtr processQueue, const MQMessageQueue& messageQueue);

  void lockMQPeriodically();
  void unlockAllMQ();
  bool lockOneMQ(const MQMessageQueue& mq);

 private:
  DefaultMQPushConsumerImpl* consumer_;
  MQMessageListener* message_listener_;

  MessageQueueLock message_queue_lock_;
  thread_pool_executor consume_executor_;
  scheduled_thread_pool_executor scheduled_executor_service_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_CONSUMEMSGSERVICE_H_
