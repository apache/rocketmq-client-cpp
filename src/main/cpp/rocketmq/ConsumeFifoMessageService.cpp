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
#include <chrono>
#include <functional>
#include <limits>
#include <system_error>

#include "TracingUtility.h"
#include "absl/strings/str_join.h"

#include "opencensus/trace/propagation/trace_context.h"
#include "opencensus/trace/span.h"

#include "ConsumeFifoMessageService.h"
#include "MixAll.h"
#include "ProcessQueue.h"
#include "PushConsumerImpl.h"
#include "rocketmq/MessageListener.h"
#include "rocketmq/Tracing.h"

ROCKETMQ_NAMESPACE_BEGIN

ConsumeFifoMessageService::ConsumeFifoMessageService(std::weak_ptr<PushConsumerImpl> consumer, int thread_count,
                                                     MessageListener message_listener)
    : ConsumeMessageServiceBase(std::move(consumer), thread_count, message_listener) {
}

void ConsumeFifoMessageService::start() {
  ConsumeMessageServiceBase::start();
  State expected = State::STARTING;
  if (state_.compare_exchange_strong(expected, State::STARTED)) {
    SPDLOG_DEBUG("ConsumeMessageOrderlyService started");
  }
}

void ConsumeFifoMessageService::shutdown() {
  // Wait till consume-message-orderly-service has fully started; otherwise, we may potentially miss closing resources
  // in concurrent scenario.
  while (State::STARTING == state_.load(std::memory_order_relaxed)) {
    absl::SleepFor(absl::Milliseconds(10));
  }

  State expected = State::STARTED;
  if (state_.compare_exchange_strong(expected, STOPPING)) {
    ConsumeMessageServiceBase::shutdown();
    SPDLOG_INFO("ConsumeMessageOrderlyService shut down");
  }
}

void ConsumeFifoMessageService::submitConsumeTask0(const std::shared_ptr<PushConsumerImpl>& consumer,
                                                   const std::weak_ptr<ProcessQueue>& process_queue,
                                                   MessageConstSharedPtr message) {
  // In case custom executor is used.
  const auto& custom_executor = consumer->customExecutor();
  if (custom_executor) {
    std::function<void(void)> consume_task =
        std::bind(&ConsumeFifoMessageService::consumeTask, this, process_queue, message);
    custom_executor(consume_task);
    SPDLOG_DEBUG("Submit FIFO consume task to custom executor");
    return;
  }

  // submit batch message
  std::function<void(void)> consume_task =
      std::bind(&ConsumeFifoMessageService::consumeTask, this, process_queue, message);
  SPDLOG_DEBUG("Submit FIFO consume task to thread pool");
  pool_->submit(consume_task);
}

void ConsumeFifoMessageService::submitConsumeTask(const std::weak_ptr<ProcessQueue>& process_queue) {
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    SPDLOG_INFO("Process queue has destructed");
    return;
  }

  auto consumer = consumer_.lock();
  if (!consumer) {
    SPDLOG_INFO("Consumer has destructed");
    return;
  }

  if (process_queue_ptr->bindFifoConsumeTask()) {
    std::vector<MessageConstSharedPtr> messages;
    process_queue_ptr->take(1, messages);
    if (!messages.empty()) {
      assert(1 == messages.size());
      submitConsumeTask0(consumer, process_queue, std::move(*messages.begin()));
    }
  }
}

void ConsumeFifoMessageService::consumeTask(const std::weak_ptr<ProcessQueue>& process_queue,
                                            MessageConstSharedPtr message) {
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    return;
  }
  const std::string& topic = message->topic();
  ConsumeResult result;
  std::shared_ptr<PushConsumerImpl> consumer = consumer_.lock();
  // consumer might have been destructed.
  if (!consumer) {
    return;
  }

  std::shared_ptr<RateLimiter<10>> rate_limiter = rateLimiter(topic);
  if (rate_limiter) {
    rate_limiter->acquire();
    SPDLOG_DEBUG("Rate-limit permit acquired");
  }

  // Record await-consumption-span
  if (message->traceContext().has_value()) {
    auto span_context = opencensus::trace::propagation::FromTraceParentHeader(message->traceContext().value());

    auto span = opencensus::trace::Span::BlankSpan();
    std::string span_name =
        consumer->resourceNamespace() + "/" + topic + " " + MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_AWAIT_OPERATION;
    if (span_context.IsValid()) {
      span = opencensus::trace::Span::StartSpanWithRemoteParent(span_name, span_context, traceSampler());
    } else {
      span = opencensus::trace::Span::StartSpan(span_name, nullptr, {traceSampler()});
    }
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_OPERATION,
                      MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_AWAIT_OPERATION);
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_OPERATION,
                      MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_AWAIT_OPERATION);
    TracingUtility::addUniversalSpanAttributes(*message, consumer->config(), span);
    // span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_AVAILABLE_TIMESTAMP, message.extension.store_time);
    absl::Time decoded_timestamp = absl::FromChrono(message->extension().decode_time);
    span.AddAnnotation(
        MixAll::SPAN_ANNOTATION_AWAIT_CONSUMPTION,
        {{MixAll::SPAN_ANNOTATION_ATTR_START_TIME,
          opencensus::trace::AttributeValueRef(absl::ToInt64Milliseconds(decoded_timestamp - absl::UnixEpoch()))}});
    span.End();
    // message.message.traceContext()
    // MessageAccessor::setTraceContext(const_cast<MessageExt&>(message),
    //                                  opencensus::trace::propagation::ToTraceParentHeader(span.context()));
  }

  auto span_context = opencensus::trace::propagation::FromTraceParentHeader(message->traceContext().value());
  auto span = opencensus::trace::Span::BlankSpan();
  std::string span_name = consumer->resourceNamespace() + "/" + message->topic() + " " +
                          MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_PROCESS_OPERATION;
  if (span_context.IsValid()) {
    span = opencensus::trace::Span::StartSpanWithRemoteParent(span_name, span_context);
  } else {
    span = opencensus::trace::Span::StartSpan(span_name);
  }
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_OPERATION,
                    MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_PROCESS_OPERATION);
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_OPERATION,
                    MixAll::SPAN_ATTRIBUTE_VALUE_MESSAGING_PROCESS_OPERATION);
  TracingUtility::addUniversalSpanAttributes(*message, consumer->config(), span);
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_ATTEMPT, std::to_string(message->extension().delivery_attempt));
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_AVAILABLE_TIMESTAMP,
                    MixAll::format(message->extension().store_time));

  // MessageAccessor::setTraceContext(const_cast<MessageExt&>(message),
  //                                  opencensus::trace::propagation::ToTraceParentHeader(span.context()));
  auto steady_start = std::chrono::steady_clock::now();

  try {
    result = message_listener_(*message);
  } catch (...) {
    result = ConsumeResult::FAILURE;
    SPDLOG_ERROR("Business FIFO callback raised an exception when consumeMessage");
  }

  switch (result) {
    case ConsumeResult::SUCCESS:
      span.SetStatus(opencensus::trace::StatusCode::OK);
      break;
    case ConsumeResult::FAILURE:
      span.SetStatus(opencensus::trace::StatusCode::UNKNOWN);
      break;
  }
  span.End();

  auto duration = std::chrono::steady_clock::now() - steady_start;

  // Log client consume-time costs
  SPDLOG_DEBUG("Business callback spent {}ms processing message[Topic={}, MessageId={}].",
               MixAll::millisecondsOf(duration), message->topic(), message->id());

  if (result == ConsumeResult::SUCCESS) {
    // Release message number and memory quota
    process_queue_ptr->release(message->body().size());

    // Ensure current message is acked before moving to the next message.
    auto callback = std::bind(&ConsumeFifoMessageService::onAck, this, process_queue, message, std::placeholders::_1);
    consumer->ack(*message, callback);
  } else {
    const Message* ptr = message.get();
    Message* raw = const_cast<Message*>(ptr);
    raw->mutableExtension().delivery_attempt++;

    if (message->extension().delivery_attempt < consumer->maxDeliveryAttempts()) {
      auto task = std::bind(&ConsumeFifoMessageService::scheduleConsumeTask, this, process_queue, message);
      consumer->schedule("Scheduled-Consume-FIFO-Message-Task", task, std::chrono::seconds(1));
    } else {
      auto callback = std::bind(&ConsumeFifoMessageService::onForwardToDeadLetterQueue, this, process_queue, message,
                                std::placeholders::_1);
      consumer->forwardToDeadLetterQueue(*message, callback);
    }
  }
}

void ConsumeFifoMessageService::onAck(const std::weak_ptr<ProcessQueue>& process_queue,
                                      MessageConstSharedPtr message,
                                      const std::error_code& ec) {
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    SPDLOG_WARN("ProcessQueue has destructed.");
    return;
  }

  if (ec) {
    SPDLOG_WARN("Failed to acknowledge FIFO message[MessageQueue={}, MsgId={}]. Cause: {}",
                process_queue_ptr->simpleName(), message->id(), ec.message());
    auto consumer = consumer_.lock();
    if (!consumer) {
      SPDLOG_WARN("Consumer instance has destructed");
      return;
    }
    auto task = std::bind(&ConsumeFifoMessageService::scheduleAckTask, this, process_queue, message);
    int32_t duration = 100;
    consumer->schedule("Ack-FIFO-Message-On-Failure", task, std::chrono::milliseconds(duration));
    SPDLOG_INFO("Scheduled to ack message[Topic={}, MessageId={}] in {}ms", message->topic(), message->id(), duration);
  } else {
    SPDLOG_DEBUG("Acknowledge FIFO message[MessageQueue={}, MsgId={}] OK", process_queue_ptr->simpleName(),
                 message->id());
    process_queue_ptr->unbindFifoConsumeTask();
    submitConsumeTask(process_queue);
  }
}

void ConsumeFifoMessageService::onForwardToDeadLetterQueue(const std::weak_ptr<ProcessQueue>& process_queue,
                                                           MessageConstSharedPtr message,
                                                           bool ok) {
  if (ok) {
    SPDLOG_DEBUG("Forward message[Topic={}, MessagId={}] to DLQ OK", message->topic(), message->id());
    auto process_queue_ptr = process_queue.lock();
    if (process_queue_ptr) {
      process_queue_ptr->unbindFifoConsumeTask();
    }
    return;
  }

  SPDLOG_INFO("Failed to forward message[topic={}, MessageId={}] to DLQ", message->topic(), message->id());
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    SPDLOG_INFO("Abort further attempts considering its process queue has destructed");
    return;
  }

  auto consumer = consumer_.lock();
  assert(consumer);

  auto task = std::bind(&ConsumeFifoMessageService::scheduleForwardDeadLetterQueueTask, this, process_queue, message);
  consumer->schedule("Scheduled-Forward-DLQ-Task", task, std::chrono::milliseconds(100));
}

void ConsumeFifoMessageService::scheduleForwardDeadLetterQueueTask(const std::weak_ptr<ProcessQueue>& process_queue,
                                                                   MessageConstSharedPtr message) {
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    return;
  }
  auto consumer = consumer_.lock();
  assert(consumer);
  auto callback = std::bind(&ConsumeFifoMessageService::onForwardToDeadLetterQueue, this, process_queue, message,
                            std::placeholders::_1);
  consumer->forwardToDeadLetterQueue(*message, callback);
}

void ConsumeFifoMessageService::scheduleAckTask(const std::weak_ptr<ProcessQueue>& process_queue,
                                                MessageConstSharedPtr message) {
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    return;
  }

  auto callback = std::bind(&ConsumeFifoMessageService::onAck, this, process_queue, message, std::placeholders::_1);
  auto consumer = consumer_.lock();
  if (consumer) {
    consumer->ack(*message, callback);
  }
}

void ConsumeFifoMessageService::scheduleConsumeTask(const std::weak_ptr<ProcessQueue>& process_queue,
                                                    MessageConstSharedPtr message) {
  auto consumer_ptr = consumer_.lock();
  if (!consumer_ptr) {
    return;
  }

  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    return;
  }

  submitConsumeTask0(consumer_ptr, process_queue_ptr, message);
  SPDLOG_INFO("Business callback failed to process FIFO messages. Re-submit consume task back to thread pool");
}

ROCKETMQ_NAMESPACE_END