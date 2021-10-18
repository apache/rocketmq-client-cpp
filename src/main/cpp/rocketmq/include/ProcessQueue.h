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

#include <memory>

#include "ConsumeMessageType.h"
#include "FilterExpression.h"
#include "ReceiveMessageCallback.h"
#include "rocketmq/MQMessageExt.h"

ROCKETMQ_NAMESPACE_BEGIN

class PushConsumer;

class ClientManager;

class ProcessQueue {
public:
  virtual ~ProcessQueue() = default;

  virtual bool expired() const = 0;

  virtual void callback(std::shared_ptr<ReceiveMessageCallback> callback) = 0;

  virtual void receiveMessage() = 0;

  virtual void nextOffset(int64_t next_offset) = 0;

  virtual bool hasPendingMessages() const = 0;

  virtual std::string topic() const = 0;

  virtual bool take(uint32_t batch_size, std::vector<MQMessageExt>& messages) = 0;

  virtual std::weak_ptr<PushConsumer> getConsumer() = 0;

  virtual const std::string& simpleName() const = 0;

  virtual MQMessageQueue getMQMessageQueue() = 0;

  virtual bool committedOffset(int64_t& offset) = 0;

  virtual void release(uint64_t body_size, int64_t offset) = 0;

  virtual void cacheMessages(const std::vector<MQMessageExt>& messages) = 0;

  virtual bool shouldThrottle() const = 0;

  virtual std::shared_ptr<ClientManager> getClientManager() = 0;

  virtual void syncIdleState() = 0;

  virtual const FilterExpression& getFilterExpression() const = 0;

  virtual bool bindFifoConsumeTask() = 0;

  virtual bool unbindFifoConsumeTask() = 0;
};

using ProcessQueueSharedPtr = std::shared_ptr<ProcessQueue>;
using ProcessQueueWeakPtr = std::weak_ptr<ProcessQueue>;

ROCKETMQ_NAMESPACE_END