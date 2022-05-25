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
#include "ConsumeMessageServiceImpl.h"

#include "ConsumeStats.h"
#include "ConsumeTask.h"
#include "LoggerImpl.h"
#include "PushConsumerImpl.h"
#include "Tag.h"
#include "ThreadPoolImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

ConsumeMessageServiceImpl::ConsumeMessageServiceImpl(std::weak_ptr<PushConsumerImpl> consumer,
                                                     int thread_count,
                                                     MessageListener message_listener)
    : state_(State::CREATED),
      thread_count_(thread_count),
      pool_(absl::make_unique<ThreadPoolImpl>(thread_count_)),
      consumer_(std::move(consumer)),
      message_listener_(message_listener) {
}

void ConsumeMessageServiceImpl::start() {
  State expected = State::CREATED;
  if (state_.compare_exchange_strong(expected, State::STARTING, std::memory_order_relaxed)) {
    pool_->start();
  }
}

void ConsumeMessageServiceImpl::shutdown() {
  State expected = State::STOPPING;
  if (state_.compare_exchange_strong(expected, State::STOPPED, std::memory_order_relaxed)) {
    pool_->shutdown();
  }
}

void ConsumeMessageServiceImpl::dispatch(std::shared_ptr<ProcessQueue> process_queue,
                                         std::vector<MessageConstSharedPtr> messages) {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return;
  }

  if (consumer->config().subscriber.fifo) {
    auto consume_task = std::make_shared<ConsumeTask>(shared_from_this(), process_queue, std::move(messages));
    pool_->submit([consume_task]() { consume_task->process(); });
    return;
  }

  for (auto message : messages) {
    auto consume_task = std::make_shared<ConsumeTask>(shared_from_this(), process_queue, message);
    pool_->submit([consume_task]() { consume_task->process(); });
  }
}

void ConsumeMessageServiceImpl::submit(std::shared_ptr<ConsumeTask> task) {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return;
  }

  pool_->submit([task]() { task->process(); });
}

void ConsumeMessageServiceImpl::ack(const Message& message, std::function<void(const std::error_code&)> cb) {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return;
  }

  std::weak_ptr<PushConsumerImpl> client(consumer_);
  const auto& topic = message.topic();
  // Collect metrics
  opencensus::stats::Record({{consumer->stats().processSuccess(), 1}},
                            {{Tag::topicTag(), topic}, {Tag::clientIdTag(), consumer->config().client_id}});
  auto callback = [cb, client, topic](const std::error_code& ec) {
    auto consumer = client.lock();
    if (ec) {
      opencensus::stats::Record({{consumer->stats().ackFailure(), 1}},
                                {{Tag::topicTag(), topic}, {Tag::clientIdTag(), consumer->config().client_id}});
    } else {
      opencensus::stats::Record({{consumer->stats().ackSuccess(), 1}},
                                {{Tag::topicTag(), topic}, {Tag::clientIdTag(), consumer->config().client_id}});
    }
    cb(ec);
  };

  consumer->ack(message, callback);
}

void ConsumeMessageServiceImpl::nack(const Message& message, std::function<void(const std::error_code&)> cb) {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return;
  }
  // Collect metrics
  opencensus::stats::Record({{consumer->stats().processFailure(), 1}},
                            {{Tag::topicTag(), message.topic()}, {Tag::clientIdTag(), consumer->config().client_id}});
  consumer->nack(message, cb);
}

void ConsumeMessageServiceImpl::forward(const Message& message, std::function<void(const std::error_code&)> cb) {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return;
  }
  consumer->forwardToDeadLetterQueue(message, cb);
}

void ConsumeMessageServiceImpl::schedule(std::shared_ptr<ConsumeTask> task, std::chrono::milliseconds delay) {
}

std::size_t ConsumeMessageServiceImpl::maxDeliveryAttempt() {
  std::shared_ptr<PushConsumerImpl> consumer = consumer_.lock();
  if (!consumer) {
    SPDLOG_WARN("The consumer has already destructed");
    return 0;
  }

  return consumer->maxDeliveryAttempts();
}

std::weak_ptr<PushConsumerImpl> ConsumeMessageServiceImpl::consumer() {
  return consumer_;
}

bool ConsumeMessageServiceImpl::preHandle(const Message& message) {
  return true;
}

bool ConsumeMessageServiceImpl::postHandle(const Message& message, ConsumeResult result) {
  return true;
}

ROCKETMQ_NAMESPACE_END