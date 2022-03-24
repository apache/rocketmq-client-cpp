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
#include "SendContext.h"

#include <system_error>

#include "opencensus/trace/propagation/trace_context.h"
#include "opencensus/trace/span.h"
#include "spdlog/spdlog.h"

#include "ProducerImpl.h"
#include "TransactionImpl.h"
#include "rocketmq/Logger.h"
#include "rocketmq/SendReceipt.h"

ROCKETMQ_NAMESPACE_BEGIN

void SendContext::onSuccess(const SendReceipt& send_receipt) noexcept {
  {
    // Mark end of send-message span.
    span_.SetStatus(opencensus::trace::StatusCode::OK);
    span_.End();
  }
  // send_receipt.traceContext(opencensus::trace::propagation::ToTraceParentHeader(span_.context()));
  std::error_code ec;
  callback_(ec, send_receipt);
}

void SendContext::onFailure(const std::error_code& ec) noexcept {
  {
    // Mark end of the send-message span.
    span_.SetStatus(opencensus::trace::StatusCode::INTERNAL);
    span_.End();
  }

  auto publisher = producer_.lock();
  if (!publisher) {
    return;
  }

  if (++attempt_times_ >= publisher->maxAttemptTimes()) {
    SPDLOG_WARN("Retried {} times, which exceeds the limit: {}", attempt_times_, publisher->maxAttemptTimes());
    callback_(ec, {});
    return;
  }

  std::shared_ptr<ProducerImpl> producer = producer_.lock();
  if (!producer) {
    SPDLOG_WARN("Producer has been destructed");
    callback_(ec, {});
    return;
  }

  if (candidates_.empty()) {
    SPDLOG_WARN("No alternative hosts to perform additional retries");
    callback_(ec, {});
    return;
  }

  auto message_queue = candidates_[attempt_times_ % candidates_.size()];
  producer->sendImpl(shared_from_this());
}

ROCKETMQ_NAMESPACE_END