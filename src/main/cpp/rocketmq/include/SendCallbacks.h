#pragma once

#include <memory>

#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "apache/rocketmq/v1/service.grpc.pb.h"
#include "opencensus/trace/span.h"

#include "TransactionImpl.h"
#include "rocketmq/AsyncCallback.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

using SendMessageRequest = apache::rocketmq::v1::SendMessageRequest;

class OnewaySendCallback : public SendCallback {
public:
  void onSuccess(SendResult &send_result) override;

  void onException(const MQException &e) override;
};

OnewaySendCallback *onewaySendCallback();

class AwaitSendCallback : public SendCallback {
public:
  void onSuccess(SendResult &send_result) override;

  void onException(const MQException &e) override;

  void await();

  explicit operator bool() const { return success_; }

  const SendResult &sendResult() const { return send_result_; }

  const std::string &errorMessage() const { return error_message_; }

private:
  absl::Mutex mtx_;
  absl::CondVar cv_;
  bool completed_{false};
  bool success_{false};
  SendResult send_result_;
  std::string error_message_;
};

class ProducerImpl;

class RetrySendCallback : public SendCallback {
public:
  RetrySendCallback(std::weak_ptr<ProducerImpl> producer, MQMessage message,
                    int max_attempt_times, SendCallback *callback,
                    std::vector<MQMessageQueue> candidates)
      : producer_(std::move(producer)), message_(std::move(message)),
        max_attempt_times_(max_attempt_times), callback_(callback),
        candidates_(std::move(candidates)),
        span_(opencensus::trace::Span::BlankSpan()) {}

  void onSuccess(SendResult &send_result) override;

  void onException(const MQException &e) override;

  MQMessage &message() { return message_; }

  int attemptTime() const { return attempt_times_; }

  const MQMessageQueue &messageQueue() const {
    int index = attempt_times_ % candidates_.size();
    return candidates_[index];
  }

  opencensus::trace::Span &span() { return span_; }

private:
  std::weak_ptr<ProducerImpl> producer_;
  MQMessage message_;
  int attempt_times_{0};
  int max_attempt_times_;
  SendCallback *callback_{nullptr};

  /**
   * @brief Once the first publish attempt failed, the following routable
   * message queues are employed.
   *
   */
  std::vector<MQMessageQueue> candidates_;

  /**
   * @brief The on-going span. Should be terminated in the callback functions.
   */
  opencensus::trace::Span span_;
};

ROCKETMQ_NAMESPACE_END