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
#include "AsyncReceiveMessageCallback.h"

#include <system_error>

#include "ClientManagerImpl.h"
#include "ConsumeMessageType.h"
#include "LoggerImpl.h"
#include "ProcessQueue.h"
#include "PushConsumerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

AsyncReceiveMessageCallback::AsyncReceiveMessageCallback(std::weak_ptr<ProcessQueue> process_queue)
    : process_queue_(std::move(process_queue)) {
  receive_message_later_ = std::bind(&AsyncReceiveMessageCallback::checkThrottleThenReceive, this);
}

void AsyncReceiveMessageCallback::onCompletion(const std::error_code& ec, const ReceiveMessageResult& result) {
  std::shared_ptr<ProcessQueue> process_queue = process_queue_.lock();
  if (!process_queue) {
    SPDLOG_INFO("Process queue has been destructed.");
    return;
  }

  auto consumer = process_queue->getConsumer().lock();
  if (!consumer) {
    return;
  }

  if (ec) {
    SPDLOG_WARN("Receive message from {} failed. Cause: {}. Attempt later.", process_queue->simpleName(), ec.message());
    receiveMessageLater();
    return;
  }

  SPDLOG_DEBUG("Receive messages from broker[host={}] returns with status=FOUND, msgListSize={}, queue={}",
               result.source_host, result.messages.size(), process_queue->simpleName());
  process_queue->cacheMessages(result.messages);
  consumer->getConsumeMessageService()->signalDispatcher();
  checkThrottleThenReceive();
}

const char* AsyncReceiveMessageCallback::RECEIVE_LATER_TASK_NAME = "receive-later-task";

void AsyncReceiveMessageCallback::checkThrottleThenReceive() {
  auto process_queue = process_queue_.lock();
  if (!process_queue) {
    SPDLOG_WARN("Process queue should have been destructed");
    return;
  }

  if (process_queue->shouldThrottle()) {
    SPDLOG_INFO("Number of messages in {} exceeds throttle threshold. Receive messages later.",
                process_queue->simpleName());
    process_queue->syncIdleState();
    receiveMessageLater();
  } else {
    // Receive message immediately
    receiveMessageImmediately();
  }
}

void AsyncReceiveMessageCallback::receiveMessageLater() {
  auto process_queue = process_queue_.lock();
  if (!process_queue) {
    return;
  }

  auto client_instance = process_queue->getClientManager();
  std::weak_ptr<AsyncReceiveMessageCallback> receive_callback_weak_ptr(shared_from_this());

  auto task = [receive_callback_weak_ptr]() {
    auto async_receive_ptr = receive_callback_weak_ptr.lock();
    if (async_receive_ptr) {
      async_receive_ptr->checkThrottleThenReceive();
    }
  };

  client_instance->getScheduler()->schedule(task, RECEIVE_LATER_TASK_NAME, std::chrono::seconds(1),
                                            std::chrono::seconds(0));
}

void AsyncReceiveMessageCallback::receiveMessageImmediately() {
  auto process_queue_shared_ptr = process_queue_.lock();
  if (!process_queue_shared_ptr) {
    SPDLOG_INFO("ProcessQueue has been released. Ignore further receive message request-response cycles");
    return;
  }

  std::shared_ptr<PushConsumerImpl> impl = process_queue_shared_ptr->getConsumer().lock();
  if (!impl) {
    SPDLOG_INFO("Owner of ProcessQueue[{}] has been released. Ignore further receive message request-response cycles",
                process_queue_shared_ptr->simpleName());
    return;
  }
  impl->receiveMessage(process_queue_shared_ptr->messageQueue(), process_queue_shared_ptr->getFilterExpression());
}

ROCKETMQ_NAMESPACE_END