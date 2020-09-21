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
#ifndef ROCKETMQ_LITEPULLCONSUMER_H_
#define ROCKETMQ_LITEPULLCONSUMER_H_

#include "TopicMessageQueueChangeListener.h"
#include "MessageSelector.h"
#include "MQMessageExt.h"
#include "MQMessageQueue.h"

namespace rocketmq {

/**
 * LitePullConsumer - interface for pull consumer
 */
class ROCKETMQCLIENT_API LitePullConsumer {
 public:
  virtual ~LitePullConsumer() = default;

 public:  // LitePullConsumer in Java
  virtual void start() = 0;
  virtual void shutdown() = 0;

  virtual bool isAutoCommit() const = 0;
  virtual void setAutoCommit(bool auto_commit) = 0;

  //
  // Automatic mode

  virtual void subscribe(const std::string& topic, const std::string& subExpression) = 0;
  virtual void subscribe(const std::string& topic, const MessageSelector& selector) = 0;

  virtual void unsubscribe(const std::string& topic) = 0;

  virtual std::vector<MQMessageExt> poll() = 0;
  virtual std::vector<MQMessageExt> poll(long timeout) = 0;

  //
  // Manually mode

  virtual std::vector<MQMessageQueue> fetchMessageQueues(const std::string& topic) = 0;

  virtual void assign(const std::vector<MQMessageQueue>& messageQueues) = 0;

  virtual void seek(const MQMessageQueue& messageQueue, int64_t offset) = 0;
  virtual void seekToBegin(const MQMessageQueue& messageQueue) = 0;
  virtual void seekToEnd(const MQMessageQueue& messageQueue) = 0;

  virtual int64_t offsetForTimestamp(const MQMessageQueue& messageQueue, int64_t timestamp) = 0;

  virtual void pause(const std::vector<MQMessageQueue>& messageQueues) = 0;
  virtual void resume(const std::vector<MQMessageQueue>& messageQueues) = 0;

  virtual void commitSync() = 0;

  virtual int64_t committed(const MQMessageQueue& messageQueue) = 0;

  virtual void registerTopicMessageQueueChangeListener(
      const std::string& topic,
      TopicMessageQueueChangeListener* topicMessageQueueChangeListener) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_LITEPULLCONSUMER_H_
