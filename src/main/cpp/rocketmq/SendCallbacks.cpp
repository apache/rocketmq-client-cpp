#include "SendCallbacks.h"

#include "ProducerImpl.h"
#include "TransactionImpl.h"
#include "opencensus/trace/propagation/trace_context.h"
#include "opencensus/trace/span.h"
#include "rocketmq/Logger.h"
#include "rocketmq/MQMessageQueue.h"
#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

void OnewaySendCallback::onFailure(const std::error_code& ec) noexcept {
  SPDLOG_WARN("Failed to one-way send message. Message: {}", ec.message());
}

void OnewaySendCallback::onSuccess(SendResult& send_result) noexcept {
  SPDLOG_DEBUG("Send message in one-way OK. MessageId: {}", send_result.getMsgId());
}

OnewaySendCallback* onewaySendCallback() {
  static OnewaySendCallback callback;
  return &callback;
}

void AwaitSendCallback::await() {
  absl::MutexLock lk(&mtx_);
  while (!completed_) {
    cv_.Wait(&mtx_);
  }
}

void AwaitSendCallback::onSuccess(SendResult& send_result) noexcept {
  send_result_ = send_result;
  completed_ = true;
  absl::MutexLock lk(&mtx_);
  cv_.SignalAll();
}

void AwaitSendCallback::onFailure(const std::error_code& ec) noexcept {
  completed_ = true;
  ec_ = ec;
  absl::MutexLock lk(&mtx_);
  cv_.SignalAll();
}

void RetrySendCallback::onSuccess(SendResult& send_result) noexcept {
  {
    // Mark end of send-message span.
    span_.SetStatus(opencensus::trace::StatusCode::OK);
    span_.End();
  }
  send_result.setMessageQueue(messageQueue());
  send_result.traceContext(opencensus::trace::propagation::ToTraceParentHeader(span_.context()));
  callback_->onSuccess(send_result);
  delete this;
}

void RetrySendCallback::onFailure(const std::error_code& ec) noexcept {
  {
    // Mark end of the send-message span.
    span_.SetStatus(opencensus::trace::StatusCode::INTERNAL);
    span_.End();
  }

  if (++attempt_times_ >= max_attempt_times_) {
    SPDLOG_WARN("Retried {} times, which exceeds the limit: {}", attempt_times_, max_attempt_times_);
    callback_->onFailure(ec);
    delete this;
    return;
  }

  std::shared_ptr<ProducerImpl> producer = producer_.lock();
  if (!producer) {
    SPDLOG_WARN("Producer has been destructed");
    callback_->onFailure(ec);
    delete this;
    return;
  }

  if (candidates_.empty()) {
    SPDLOG_WARN("No alternative hosts to perform additional retries");
    callback_->onFailure(ec);
    delete this;
    return;
  }

  MQMessageQueue message_queue = candidates_[attempt_times_ % candidates_.size()];
  producer->sendImpl(this);
}

ROCKETMQ_NAMESPACE_END