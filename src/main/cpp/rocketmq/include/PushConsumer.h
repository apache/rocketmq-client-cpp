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

#include <functional>
#include <memory>
#include <system_error>

#include "Consumer.h"
#include "ProcessQueue.h"
#include "rocketmq/Executor.h"
#include "rocketmq/MessageListener.h"
#include "rocketmq/MessageModel.h"

ROCKETMQ_NAMESPACE_BEGIN

class PushConsumer : virtual public Consumer {
public:
  ~PushConsumer() override = default;

  virtual void iterateProcessQueue(const std::function<void(ProcessQueueSharedPtr)>& cb) = 0;

  virtual MessageModel messageModel() const = 0;

  virtual void ack(const MQMessageExt& msg, const std::function<void(const std::error_code&)>& callback) = 0;

  virtual void forwardToDeadLetterQueue(const MQMessageExt& message, const std::function<void(bool)>& cb) = 0;

  virtual const Executor& customExecutor() const = 0;

  virtual uint32_t consumeBatchSize() const = 0;

  virtual int32_t maxDeliveryAttempts() const = 0;

  virtual void updateOffset(const MQMessageQueue& message_queue, int64_t offset) = 0;

  virtual void nack(const MQMessageExt& message, const std::function<void(const std::error_code&)>& callback) = 0;

  virtual bool receiveMessage(const MQMessageQueue& message_queue, const FilterExpression& filter_expression) = 0;

  virtual MessageListener* messageListener() = 0;
};

using PushConsumerSharedPtr = std::shared_ptr<PushConsumer>;
using PushConsumerWeakPtr = std::weak_ptr<PushConsumer>;

ROCKETMQ_NAMESPACE_END