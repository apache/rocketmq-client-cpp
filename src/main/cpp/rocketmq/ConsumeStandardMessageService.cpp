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
#include "ConsumeStandardMessageService.h"

#include <limits>
#include <string>
#include <system_error>
#include <utility>

#include "LoggerImpl.h"
#include "MessageExt.h"
#include "MixAll.h"
#include "Protocol.h"
#include "PushConsumerImpl.h"
#include "TracingUtility.h"
#include "UtilAll.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_join.h"
#include "absl/time/time.h"
#include "absl/types/span.h"
#include "rocketmq/Message.h"
#include "rocketmq/MessageListener.h"
#include "rocketmq/Tracing.h"

ROCKETMQ_NAMESPACE_BEGIN

ConsumeStandardMessageService::ConsumeStandardMessageService(std::weak_ptr<PushConsumerImpl> consumer, int thread_count,
                                                             MessageListener message_listener)
    : ConsumeMessageServiceBase(std::move(consumer), thread_count, message_listener) {
}

void ConsumeStandardMessageService::start() {
  ConsumeMessageServiceBase::start();
  State expected = State::STARTING;
  if (state_.compare_exchange_strong(expected, State::STARTED)) {
    SPDLOG_DEBUG("ConsumeMessageConcurrentlyService started");
  }
}

void ConsumeStandardMessageService::shutdown() {
  while (State::STARTING == state_.load(std::memory_order_relaxed)) {
    absl::SleepFor(absl::Milliseconds(10));
  }

  State expected = State::STARTED;
  if (state_.compare_exchange_strong(expected, State::STOPPING)) {
    ConsumeMessageServiceBase::shutdown();
    SPDLOG_DEBUG("ConsumeMessageConcurrentlyService shut down");
  }
}

void ConsumeStandardMessageService::submitConsumeTask(const std::weak_ptr<ProcessQueue>& process_queue) {
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    SPDLOG_WARN("ProcessQueue was destructed. It is likely that client should have shutdown.");
    return;
  }
  std::shared_ptr<PushConsumerImpl> consumer = process_queue_ptr->getConsumer().lock();

  if (!consumer) {
    return;
  }

  std::string topic = process_queue_ptr->topic();
  bool has_more = true;
  std::weak_ptr<ConsumeStandardMessageService> service(shared_from_this());
  while (has_more) {
    std::vector<MessageConstSharedPtr> messages;
    uint32_t batch_size = 1;
    has_more = process_queue_ptr->take(batch_size, messages);
    if (messages.empty()) {
      assert(!has_more);
      break;
    }

    // In case custom executor is used.
    const Executor& custom_executor = consumer->customExecutor();
    if (custom_executor) {
      std::function<void(void)> consume_task =
          std::bind(&ConsumeStandardMessageService::consumeTask, service, process_queue, messages);
      custom_executor(consume_task);
      SPDLOG_DEBUG("Submit consumer task to custom executor with message-batch-size={}", messages.size());
      continue;
    }

    // submit batch message
    std::function<void(void)> consume_task =
        std::bind(&ConsumeStandardMessageService::consumeTask, service, process_queue_ptr, messages);
    SPDLOG_DEBUG("Submit consumer task to thread pool with message-batch-size={}", messages.size());
    pool_->submit(consume_task);
  }
}

void ConsumeStandardMessageService::consumeTask(std::weak_ptr<ConsumeStandardMessageService> service,
                                                const std::weak_ptr<ProcessQueue>& process_queue,
                                                const std::vector<MessageConstSharedPtr>& msgs) {
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr || msgs.empty()) {
    return;
  }

  auto svc = service.lock();
  if (!svc) {
    return;
  }

  auto process_queue_shared_ptr = process_queue.lock();
  if (!process_queue_shared_ptr) {
    return;
  }

  svc->consume(process_queue_shared_ptr, msgs);
}

void ConsumeStandardMessageService::consume(const std::shared_ptr<ProcessQueue>& process_queue,
                                            const std::vector<MessageConstSharedPtr>& msgs) {
  std::string topic = (*msgs.begin())->topic();
  ConsumeResult status;

  std::shared_ptr<PushConsumerImpl> consumer = consumer_.lock();
  // consumer might have been destructed.
  if (!consumer) {
    return;
  }

  std::shared_ptr<RateLimiter<10>> rate_limiter = rateLimiter(topic);
  if (rate_limiter) {
    // Acquire permits one-by-one to avoid large batch hungry issue.
    for (std::size_t i = 0; i < msgs.size(); i++) {
      rate_limiter->acquire();
    }
    SPDLOG_DEBUG("{} rate-limit permits acquired", msgs.size());
  }

  // Record await-consumption-span
  {
    for (const auto& msg : msgs) {
      if (!msg->traceContext().has_value()) {
        continue;
      }
      auto span_context = opencensus::trace::propagation::FromTraceParentHeader(msg->traceContext().value());

      auto span = opencensus::trace::Span::BlankSpan();
      std::string span_name = consumer->resourceNamespace() + "/" + msg->topic() + " " +
                              MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_AWAIT_OPERATION;
      if (span_context.IsValid()) {
        span = opencensus::trace::Span::StartSpanWithRemoteParent(span_name, span_context, traceSampler());
      } else {
        span = opencensus::trace::Span::StartSpan(span_name, nullptr, {traceSampler()});
      }
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_OPERATION,
                        MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_AWAIT_OPERATION);
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_OPERATION,
                        MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_AWAIT_OPERATION);
      TracingUtility::addUniversalSpanAttributes(*msg, consumer->config(), span);
      // span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_AVAILABLE_TIMESTAMP, msg.getStoreTimestamp());
      absl::Time decoded_timestamp = absl::FromChrono(msg->extension().decode_time);
      span.AddAnnotation(
          MixAll::SPAN_ANNOTATION_AWAIT_CONSUMPTION,
          {{MixAll::SPAN_ANNOTATION_ATTR_START_TIME,
            opencensus::trace::AttributeValueRef(absl::ToInt64Milliseconds(decoded_timestamp - absl::UnixEpoch()))}});
      span.End();
      // MessageAccessor::setTraceContext(const_cast<MessageExt&>(msg),
      //                                  opencensus::trace::propagation::ToTraceParentHeader(span.context()));
    }
  }

  // Trace start of consume message
  std::vector<opencensus::trace::Span> spans;
  {
    for (const auto& msg : msgs) {
      if (!msg->traceContext().has_value()) {
        continue;
      }
      auto span_context = opencensus::trace::propagation::FromTraceParentHeader(msg->traceContext().value());
      auto span = opencensus::trace::Span::BlankSpan();
      std::string span_name = consumer->resourceNamespace() + "/" + msg->topic() + " " +
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
      TracingUtility::addUniversalSpanAttributes(*msg, consumer->config(), span);
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_ATTEMPT, msg->extension().delivery_attempt);
      // span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_AVAILABLE_TIMESTAMP, msg.extension.store_time);
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_BATCH_SIZE, msgs.size());
      spans.emplace_back(std::move(span));
      // MessageAccessor::setTraceContext(const_cast<MessageExt&>(msg),
      //                                  opencensus::trace::propagation::ToTraceParentHeader(span.context()));
    }
  }

  auto steady_start = std::chrono::steady_clock::now();

  try {
    // TODO:
    status = message_listener_(**msgs.begin());
  } catch (...) {
    status = ConsumeResult::FAILURE;
    SPDLOG_ERROR("Business callback raised an exception when consumeMessage");
  }

  auto duration = std::chrono::steady_clock::now() - steady_start;

  // Trace end of consumption
  {
    for (auto& span : spans) {
      switch (status) {
        case ConsumeResult::SUCCESS:
          span.SetStatus(opencensus::trace::StatusCode::OK);
          break;
        case ConsumeResult::FAILURE:
          span.SetStatus(opencensus::trace::StatusCode::UNKNOWN);
          break;
      }
      span.End();
    }
  }

  // Log client consume-time costs
  SPDLOG_DEBUG("Business callback spent {}ms processing {} messages.", MixAll::millisecondsOf(duration), msgs.size());

  for (const auto& msg : msgs) {
    const std::string& message_id = msg->id();

    // Release message number and memory quota
    process_queue->release(msg->body().size());

    if (status == ConsumeResult::SUCCESS) {
      auto callback = [process_queue, message_id](const std::error_code& ec) {
        if (ec) {
          SPDLOG_WARN("Failed to acknowledge message[MessageQueue={}, MsgId={}]. Cause: {}",
                      process_queue->simpleName(), message_id, ec.message());
        } else {
          SPDLOG_DEBUG("Acknowledge message[MessageQueue={}, MsgId={}] OK", process_queue->simpleName(), message_id);
        }
      };
      consumer->ack(*msg, callback);
    } else {
      auto callback = [process_queue, message_id](const std::error_code& ec) {
        if (ec) {
          SPDLOG_WARN(
              "Failed to negative acknowledge message[MessageQueue={}, MsgId={}]. Cause: {} Message will be "
              "re-consumed after default invisible time",
              process_queue->simpleName(), message_id, ec.message());
          return;
        }

        SPDLOG_DEBUG("Nack message[MessageQueue={}, MsgId={}] OK", process_queue->simpleName(), message_id);
      };
      consumer->nack(*msg, callback);
    }
  }
}

ROCKETMQ_NAMESPACE_END