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
#include <limits>
#include <string>
#include <system_error>
#include <utility>

#include "TracingUtility.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_join.h"
#include "absl/time/time.h"
#include "absl/types/span.h"
#include "opencensus/trace/propagation/trace_context.h"
#include "opencensus/trace/span.h"

#include "ConsumeStandardMessageService.h"
#include "LoggerImpl.h"
#include "MessageAccessor.h"
#include "MixAll.h"
#include "OtlpExporter.h"
#include "Protocol.h"
#include "PushConsumer.h"
#include "UtilAll.h"
#include "rocketmq/ConsumeType.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQMessageExt.h"
#include "rocketmq/MessageListener.h"

ROCKETMQ_NAMESPACE_BEGIN

ConsumeStandardMessageService::ConsumeStandardMessageService(std::weak_ptr<PushConsumer> consumer, int thread_count,
                                                             MessageListener* message_listener_ptr)
    : ConsumeMessageServiceBase(std::move(consumer), thread_count, message_listener_ptr) {
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

void ConsumeStandardMessageService::submitConsumeTask(const ProcessQueueWeakPtr& process_queue) {
  ProcessQueueSharedPtr process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    SPDLOG_WARN("ProcessQueue was destructed. It is likely that client should have shutdown.");
    return;
  }
  std::shared_ptr<PushConsumer> consumer = process_queue_ptr->getConsumer().lock();

  if (!consumer) {
    return;
  }

  std::string topic = process_queue_ptr->topic();
  bool has_more = true;
  while (has_more) {
    std::vector<MQMessageExt> messages;
    uint32_t batch_size = consumer->consumeBatchSize();
    has_more = process_queue_ptr->take(batch_size, messages);
    if (messages.empty()) {
      assert(!has_more);
      break;
    }

    // In case custom executor is used.
    const Executor& custom_executor = consumer->customExecutor();
    if (custom_executor) {
      std::function<void(void)> consume_task =
          std::bind(&ConsumeStandardMessageService::consumeTask, this, process_queue, messages);
      custom_executor(consume_task);
      SPDLOG_DEBUG("Submit consumer task to custom executor with message-batch-size={}", messages.size());
      continue;
    }

    // submit batch message
    std::function<void(void)> consume_task =
        std::bind(&ConsumeStandardMessageService::consumeTask, this, process_queue_ptr, messages);
    SPDLOG_DEBUG("Submit consumer task to thread pool with message-batch-size={}", messages.size());
    pool_->submit(consume_task);
  }
}

MessageListenerType ConsumeStandardMessageService::messageListenerType() {
  return MessageListenerType::STANDARD;
}

void ConsumeStandardMessageService::consumeTask(const ProcessQueueWeakPtr& process_queue,
                                                const std::vector<MQMessageExt>& msgs) {
  ProcessQueueSharedPtr process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr || msgs.empty()) {
    return;
  }
  std::string topic = msgs.begin()->getTopic();
  ConsumeMessageResult status;
  std::shared_ptr<PushConsumer> consumer = consumer_.lock();
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
      auto span_context = opencensus::trace::propagation::FromTraceParentHeader(msg.traceContext());

      auto span = opencensus::trace::Span::BlankSpan();
      std::string span_name = consumer->resourceNamespace() + "/" + msg.getTopic() + " " +
                              MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_AWAIT_OPERATION;
      if (span_context.IsValid()) {
        span = opencensus::trace::Span::StartSpanWithRemoteParent(span_name, span_context, &Samplers::always());
      } else {
        span = opencensus::trace::Span::StartSpan(span_name, nullptr, {&Samplers::always()});
      }
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_OPERATION,
                        MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_AWAIT_OPERATION);
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_OPERATION,
                        MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_AWAIT_OPERATION);
      TracingUtility::addUniversalSpanAttributes(msg, *consumer, span);
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_AVAILABLE_TIMESTAMP, msg.getStoreTimestamp());
      absl::Time decoded_timestamp = MessageAccessor::decodedTimestamp(msg);
      span.AddAnnotation(
          MixAll::SPAN_ANNOTATION_AWAIT_CONSUMPTION,
          {{MixAll::SPAN_ANNOTATION_ATTR_START_TIME,
            opencensus::trace::AttributeValueRef(absl::ToInt64Milliseconds(decoded_timestamp - absl::UnixEpoch()))}});
      span.End();
      MessageAccessor::setTraceContext(const_cast<MQMessageExt&>(msg),
                                       opencensus::trace::propagation::ToTraceParentHeader(span.context()));
    }
  }

  // Trace start of consume message
  std::vector<opencensus::trace::Span> spans;
  {
    for (const auto& msg : msgs) {
      auto span_context = opencensus::trace::propagation::FromTraceParentHeader(msg.traceContext());
      auto span = opencensus::trace::Span::BlankSpan();
      std::string span_name = consumer->resourceNamespace() + "/" + msg.getTopic() + " " +
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
      TracingUtility::addUniversalSpanAttributes(msg, *consumer, span);
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_ATTEMPT, msg.getDeliveryAttempt());
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_AVAILABLE_TIMESTAMP, msg.getStoreTimestamp());
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_BATCH_SIZE, msgs.size());
      spans.emplace_back(std::move(span));
      MessageAccessor::setTraceContext(const_cast<MQMessageExt&>(msg),
                                       opencensus::trace::propagation::ToTraceParentHeader(span.context()));
    }
  }

  auto steady_start = std::chrono::steady_clock::now();

  try {
    assert(nullptr != message_listener_);
    auto message_listener = dynamic_cast<StandardMessageListener*>(message_listener_);
    assert(message_listener);
    status = message_listener->consumeMessage(msgs);
  } catch (...) {
    status = ConsumeMessageResult::FAILURE;
    SPDLOG_ERROR("Business callback raised an exception when consumeMessage");
  }

  auto duration = std::chrono::steady_clock::now() - steady_start;

  // Trace end of consumption
  {
    for (auto& span : spans) {
      switch (status) {
        case ConsumeMessageResult::SUCCESS:
          span.SetStatus(opencensus::trace::StatusCode::OK);
          break;
        case ConsumeMessageResult::FAILURE:
          span.SetStatus(opencensus::trace::StatusCode::UNKNOWN);
          break;
      }
      span.End();
    }
  }

  // Log client consume-time costs
  SPDLOG_DEBUG("Business callback spent {}ms processing {} messages.", MixAll::millisecondsOf(duration), msgs.size());

  if (MessageModel::CLUSTERING == consumer->messageModel()) {
    for (const auto& msg : msgs) {
      const std::string& message_id = msg.getMsgId();

      // Release message number and memory quota
      process_queue_ptr->release(msg.getBody().size(), msg.getQueueOffset());

      if (status == ConsumeMessageResult::SUCCESS) {
        auto callback = [process_queue_ptr, message_id](const std::error_code& ec) {
          if (ec) {
            SPDLOG_WARN("Failed to acknowledge message[MessageQueue={}, MsgId={}]. Cause: {}",
                        process_queue_ptr->simpleName(), message_id, ec.message());
          } else {
            SPDLOG_DEBUG("Acknowledge message[MessageQueue={}, MsgId={}] OK", process_queue_ptr->simpleName(),
                         message_id);
          }
        };
        consumer->ack(msg, callback);
      } else {
        auto callback = [process_queue_ptr, message_id](const std::error_code& ec) {
          if (ec) {
            SPDLOG_WARN("Failed to negative acknowledge message[MessageQueue={}, MsgId={}]. Cause: {} Message will be "
                        "re-consumed after default invisible time",
                        process_queue_ptr->simpleName(), message_id, ec.message());
            return;
          }

          SPDLOG_DEBUG("Nack message[MessageQueue={}, MsgId={}] OK", process_queue_ptr->simpleName(), message_id);
        };
        consumer->nack(msg, callback);
      }
    }

  } else if (MessageModel::BROADCASTING == consumer->messageModel()) {
    int64_t committed_offset;
    if (process_queue_ptr->committedOffset(committed_offset)) {
      consumer->updateOffset(process_queue_ptr->getMQMessageQueue(), committed_offset);
    }
  }
}

ROCKETMQ_NAMESPACE_END