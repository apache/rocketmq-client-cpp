#include "SendCallbacks.h"

#include "rocketmq/Logger.h"
#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

void OnewaySendCallback::onException(const MQException& e) {
  SPDLOG_WARN("Failed to one-way send message. Message: {}", e.what());
}

void OnewaySendCallback::onSuccess(const SendResult& send_result) {
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

void AwaitSendCallback::onSuccess(const SendResult& send_result) {
  send_result_ = send_result;
  success_ = true;
  completed_ = true;
  absl::MutexLock lk(&mtx_);
  cv_.SignalAll();
}

void AwaitSendCallback::onException(const MQException& e) {
  success_ = false;
  error_message_ = e.what();
  completed_ = true;
  absl::MutexLock lk(&mtx_);
  cv_.SignalAll();
}

void RetrySendCallback::onSuccess(const SendResult& send_result) {
  callback_->onSuccess(send_result);
  SPDLOG_DEBUG("");
  delete this;
}

void RetrySendCallback::onException(const MQException& e) {
  if (++attempt_times_ >= max_attempt_times_) {
    SPDLOG_WARN("Retried {} times, which exceeds the limit: {}", attempt_times_, max_attempt_times_);
    callback_->onException(e);
    delete this;
    return;
  }

  std::shared_ptr<ClientManager> client = client_manager_.lock();
  if (!client) {
    SPDLOG_WARN("Client instance has destructed");
    callback_->onException(e);
    delete this;
    return;
  }

  if (candidates_.empty()) {
    SPDLOG_WARN("No alternative hosts to perform additional retries");
    callback_->onException(e);
    delete this;
    return;
  }

  const std::string& host = candidates_[attempt_times_ % candidates_.size()].serviceAddress();
  SPDLOG_DEBUG("Retry-send message to {} for {} times", host, attempt_times_);

  // TODO: Sync target broker-name and partition-id with current partition.
  request_.mutable_message()->mutable_system_attribute()->set_partition_id(
      candidates_[attempt_times_ % candidates_.size()].getQueueId());

  client->send(host, metadata_, request_, this);
}

ROCKETMQ_NAMESPACE_END