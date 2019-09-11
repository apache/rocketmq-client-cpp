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
#ifndef __MQ_CONSUMER_H__
#define __MQ_CONSUMER_H__

#include <memory>
#include <string>

#include "AsyncCallback.h"
#include "ConsumeType.h"
#include "MQAdmin.h"
#include "MQMessageListener.h"
#include "SubscriptionData.h"

namespace rocketmq {

class ConsumerRunningInfo;

/**
 * MQ Consumer API
 */
class ROCKETMQCLIENT_API MQConsumer : virtual public MQAdmin {
 public:
  virtual ~MQConsumer() = default;

 public:  // MQConsumer in Java
  virtual bool sendMessageBack(MQMessageExt& msg, int delayLevel) = 0;
  virtual void fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) = 0;

 public:  // MQConsumerInner in Java Client
  virtual std::string groupName() const = 0;
  virtual MessageModel messageModel() const = 0;
  virtual ConsumeType consumeType() const = 0;
  virtual ConsumeFromWhere consumeFromWhere() const = 0;
  virtual std::vector<SubscriptionData> subscriptions() const = 0;

  virtual void doRebalance() = 0;
  virtual void persistConsumerOffset() = 0;
  virtual void updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info) = 0;
  virtual ConsumerRunningInfo* consumerRunningInfo() = 0;
};

class ROCKETMQCLIENT_API MQPushConsumer : public MQConsumer {
 public:  // MQPushConsumer in Java
  // [[deprecated]]
  virtual void registerMessageListener(MQMessageListener* messageListener) = 0;
  virtual void registerMessageListener(MessageListenerConcurrently* messageListener) = 0;
  virtual void registerMessageListener(MessageListenerOrderly* messageListener) = 0;

  virtual void subscribe(const std::string& topic, const std::string& subExpression) = 0;
  // virtual void subscribe(const std::string& topic, MessageSelector* selector) = 0;

  virtual void suspend() = 0;
  virtual void resume() = 0;
};

class ROCKETMQCLIENT_API MQPullConsumer : public MQConsumer {
 public:
  virtual PullResult pull(const MQMessageQueue& mq, const std::string& subExpression, int64_t offset, int maxNums) = 0;
  virtual void pull(const MQMessageQueue& mq,
                    const std::string& subExpression,
                    int64_t offset,
                    int maxNums,
                    PullCallback* pullCallback) = 0;
};

class ROCKETMQCLIENT_API DefaultMQConsumerConfig {
 public:
  DefaultMQConsumerConfig() : m_messageModel(CLUSTERING) {}

  MessageModel getMessageModel() const { return m_messageModel; }
  void setMessageModel(MessageModel messageModel) { m_messageModel = messageModel; }

 protected:
  MessageModel m_messageModel;
};

}  // namespace rocketmq

#endif  // __MQ_CONSUMER_H__
