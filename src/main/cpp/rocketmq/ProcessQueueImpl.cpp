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
#include "ProcessQueueImpl.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <system_error>
#include <utility>

#include "ClientManagerImpl.h"
#include "MetadataConstants.h"
#include "Protocol.h"
#include "PushConsumerImpl.h"
#include "ReceiveMessageResult.h"
#include "Signature.h"
#include "include/AsyncReceiveMessageCallback.h"
#include "rocketmq/MessageListener.h"

using namespace std::chrono;

ROCKETMQ_NAMESPACE_BEGIN

ProcessQueueImpl::ProcessQueueImpl(rmq::MessageQueue message_queue, FilterExpression filter_expression,
                                   std::weak_ptr<PushConsumerImpl> consumer,
                                   std::shared_ptr<ClientManager> client_instance)
    : message_queue_(std::move(message_queue)), filter_expression_(std::move(filter_expression)),
      invisible_time_(MixAll::millisecondsOf(MixAll::DEFAULT_INVISIBLE_TIME_)),
      simple_name_(simpleNameOf(message_queue_)), consumer_(std::move(consumer)),
      client_manager_(std::move(client_instance)), cached_message_quantity_(0), cached_message_memory_(0) {
  SPDLOG_DEBUG("Created ProcessQueue={}", simpleName());
}

ProcessQueueImpl::~ProcessQueueImpl() {
  SPDLOG_INFO("ProcessQueue={} should have been re-balanced away, thus, is destructed", simpleName());
}

void ProcessQueueImpl::callback(std::shared_ptr<AsyncReceiveMessageCallback> callback) {
  receive_callback_ = callback;
}

bool ProcessQueueImpl::expired() const {
  auto duration = std::chrono::steady_clock::now() - idle_since_;
  if (duration > MixAll::PROCESS_QUEUE_EXPIRATION_THRESHOLD_) {
    SPDLOG_WARN("ProcessQueue={} is expired. It remains idle for {}ms", simpleName(), MixAll::millisecondsOf(duration));
    return true;
  }
  return false;
}

bool ProcessQueueImpl::shouldThrottle() const {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return false;
  }

  std::size_t quantity = cached_message_quantity_.load(std::memory_order_relaxed);
  uint32_t quantity_threshold = consumer->maxCachedMessageQuantity();
  uint64_t memory_threshold = consumer->maxCachedMessageMemory();
  bool need_throttle = quantity >= quantity_threshold;
  if (need_throttle) {
    SPDLOG_INFO("{}: Number of locally cached messages is {}, which exceeds threshold={}", simple_name_, quantity,
                quantity_threshold);
    return true;
  }

  if (memory_threshold) {
    uint64_t bytes = cached_message_memory_.load(std::memory_order_relaxed);
    need_throttle = bytes >= memory_threshold;
    if (need_throttle) {
      SPDLOG_INFO("{}: Locally cached messages take {} bytes, which exceeds threshold={}", simple_name_, bytes,
                  memory_threshold);
      return true;
    }
  }
  return false;
}

void ProcessQueueImpl::receiveMessage() {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return;
  }

  popMessage();
}

void ProcessQueueImpl::popMessage() {
  rmq::ReceiveMessageRequest request;
  absl::flat_hash_map<std::string, std::string> metadata;
  auto consumer_client = consumer_.lock();
  if (!consumer_client) {
    return;
  }
  Signature::sign(consumer_client->config(), metadata);
  wrapPopMessageRequest(metadata, request);
  syncIdleState();
  SPDLOG_DEBUG("Try to pop message from {}", simpleNameOf(message_queue_));

  std::weak_ptr<AsyncReceiveMessageCallback> cb{receive_callback_};
  auto callback = [cb](const std::error_code& ec, const ReceiveMessageResult& result) {
    auto recv_cb = cb.lock();
    if (recv_cb) {
      recv_cb->onCompletion(ec, result);
    }
  };

  client_manager_->receiveMessage(
      urlOf(message_queue_), metadata, request,
      absl::ToChronoMilliseconds(consumer_client->config().subscriber.polling_timeout), callback);
}

bool ProcessQueueImpl::hasPendingMessages() const {
  absl::MutexLock lk(&messages_mtx_);
  return !cached_messages_.empty();
}

void ProcessQueueImpl::cacheMessages(const std::vector<MessageConstSharedPtr>& messages) {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return;
  }

  {
    absl::MutexLock messages_lock_guard(&messages_mtx_);
    for (const auto& message : messages) {
      const std::string& msg_id = message->id();
      if (!filter_expression_.accept(*message)) {
        const std::string& topic = message->topic();
        auto callback = [topic, msg_id](const std::error_code& ec) {
          if (ec) {
            SPDLOG_WARN(
                "Failed to ack message[Topic={}, MsgId={}] directly as it fails to pass filter expression. Cause: {}",
                topic, msg_id, ec.message());
          } else {
            SPDLOG_DEBUG("Ack message[Topic={}, MsgId={}] directly as it fails to pass filter expression", topic,
                         msg_id);
          }
        };
        consumer->ack(*message, callback);
        continue;
      }
      cached_messages_.emplace_back(message);
      cached_message_quantity_.fetch_add(1, std::memory_order_relaxed);
      cached_message_memory_.fetch_add(message->body().size(), std::memory_order_relaxed);
    }
  }
}

bool ProcessQueueImpl::take(uint32_t batch_size, std::vector<MessageConstSharedPtr>& messages) {
  absl::MutexLock lock(&messages_mtx_);
  if (cached_messages_.empty()) {
    return false;
  }

  for (auto it = cached_messages_.begin(); it != cached_messages_.end();) {
    if (0 == batch_size--) {
      break;
    }
    messages.push_back(*it);
    it = cached_messages_.erase(it);
  }
  return !cached_messages_.empty();
}

void ProcessQueueImpl::release(uint64_t body_size) {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return;
  }

  cached_message_quantity_.fetch_sub(1);
  cached_message_memory_.fetch_sub(body_size);
}

void ProcessQueueImpl::wrapFilterExpression(rmq::FilterExpression* filter_expression) {
  assert(filter_expression);
  auto consumer = consumer_.lock();
  if (!consumer) {
    return;
  }
  auto&& optional = consumer->getFilterExpression(message_queue_.topic().name());
  if (optional.has_value()) {
    auto expression = optional.value();
    switch (expression.type_) {
      case TAG:
        filter_expression->set_type(rmq::FilterType::TAG);
        filter_expression->set_expression(expression.content_);
        break;
      case SQL92:
        filter_expression->set_type(rmq::FilterType::SQL);
        filter_expression->set_expression(expression.content_);
        break;
    }
  } else {
    filter_expression->set_type(rmq::FilterType::TAG);
    filter_expression->set_expression("*");
  }
}

void ProcessQueueImpl::wrapPopMessageRequest(absl::flat_hash_map<std::string, std::string>& metadata,
                                             rmq::ReceiveMessageRequest& request) {
  std::shared_ptr<PushConsumerImpl> consumer = consumer_.lock();
  assert(consumer);
  request.mutable_group()->CopyFrom(consumer->config().subscriber.group);
  request.mutable_message_queue()->CopyFrom(message_queue_);

  wrapFilterExpression(request.mutable_filter_expression());

  // Batch size
  request.set_batch_size(consumer->receiveBatchSize());

  // Let server to automatically extend invisible duration.
  request.set_auto_renew(true);

  // Set invisible time
  request.mutable_invisible_duration()->set_seconds(
      std::chrono::duration_cast<std::chrono::seconds>(invisible_time_).count());
  auto fraction = invisible_time_ - std::chrono::duration_cast<std::chrono::seconds>(invisible_time_);
  int32_t nano_seconds = static_cast<int32_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(fraction).count());
  request.mutable_invisible_duration()->set_nanos(nano_seconds);
}

std::weak_ptr<PushConsumerImpl> ProcessQueueImpl::getConsumer() {
  return consumer_;
}

std::shared_ptr<ClientManager> ProcessQueueImpl::getClientManager() {
  return client_manager_;
}

const FilterExpression& ProcessQueueImpl::getFilterExpression() const {
  return filter_expression_;
}

ROCKETMQ_NAMESPACE_END