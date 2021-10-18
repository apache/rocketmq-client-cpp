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
#pragma once

#include "MQMessageExt.h"
#include "MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class ConsumeMessageResult : uint8_t
{
  SUCCESS = 0,
  FAILURE = 1
};

enum class MessageListenerType : uint8_t
{
  STANDARD = 0,
  FIFO = 1
};

class MessageListener {
public:
  virtual ~MessageListener() = default;

  virtual MessageListenerType listenerType() = 0;
};

class StandardMessageListener : public MessageListener {
public:
  virtual ConsumeMessageResult consumeMessage(const std::vector<MQMessageExt>& msgs) = 0;

  MessageListenerType listenerType() override {
    return MessageListenerType::STANDARD;
  }
};

class FifoMessageListener : public MessageListener {
public:
  MessageListenerType listenerType() override {
    return MessageListenerType::FIFO;
  }

  virtual ConsumeMessageResult consumeMessage(const MQMessageExt& msgs) = 0;
};

ROCKETMQ_NAMESPACE_END