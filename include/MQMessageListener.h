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
#ifndef ROCKETMQ_MQMESSAGELISTENER_H_
#define ROCKETMQ_MQMESSAGELISTENER_H_

#include "MQMessageExt.h"

namespace rocketmq {

enum ConsumeStatus {
  // consume success, msg will be cleard from memory
  CONSUME_SUCCESS,
  // consume fail, but will be re-consume by call messageLisenter again
  RECONSUME_LATER
};

enum MessageListenerType { messageListenerDefaultly = 0, messageListenerOrderly = 1, messageListenerConcurrently = 2 };

/**
 * MQMessageListener - listener interface for MQPushConsumer
 */
class ROCKETMQCLIENT_API MQMessageListener {
 public:
  virtual ~MQMessageListener() = default;

  virtual MessageListenerType getMessageListenerType() { return messageListenerDefaultly; }

  virtual ConsumeStatus consumeMessage(std::vector<MQMessageExt>& msgs) { return RECONSUME_LATER; };
};

/**
 * MessageListenerConcurrently - listener interface for MQPushConsumer in Concurrently mode
 */
class ROCKETMQCLIENT_API MessageListenerConcurrently : virtual public MQMessageListener {
 public:
  MessageListenerType getMessageListenerType() override final { return messageListenerConcurrently; }
};

/**
 * MessageListenerOrderly - listener interface for MQPushConsumer in Orderly mode
 */
class ROCKETMQCLIENT_API MessageListenerOrderly : virtual public MQMessageListener {
 public:
  MessageListenerType getMessageListenerType() override final { return messageListenerOrderly; }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQMESSAGELISTENER_H_
