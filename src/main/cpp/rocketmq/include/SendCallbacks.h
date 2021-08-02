#pragma once

#include "ClientManager.h"
#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "apache/rocketmq/v1/service.grpc.pb.h"
#include "rocketmq/AsyncCallback.h"
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN

using SendMessageRequest = apache::rocketmq::v1::SendMessageRequest;

class OnewaySendCallback : public SendCallback {
public:
  void onSuccess(const SendResult& send_result) override;

  void onException(const MQException& e) override;
};

OnewaySendCallback* onewaySendCallback();

class AwaitSendCallback : public SendCallback {
public:
  void onSuccess(const SendResult& send_result) override;

  void onException(const MQException& e) override;

  void await();

  explicit operator bool() const { return success_; }

  const SendResult& sendResult() const { return send_result_; }

  const std::string& errorMessage() const { return error_message_; }

private:
  absl::Mutex mtx_;
  absl::CondVar cv_;
  bool completed_{false};
  bool success_{false};
  SendResult send_result_;
  std::string error_message_;
};

class RetrySendCallback : public SendCallback {
public:
  RetrySendCallback(std::weak_ptr<ClientManager> client_manager, absl::flat_hash_map<std::string, std::string> metadata,
                    SendMessageRequest request, int max_attempt_times, SendCallback* callback,
                    std::vector<MQMessageQueue> candidates)
      : client_manager_(std::move(client_manager)), metadata_(std::move(metadata)), request_(std::move(request)),
        max_attempt_times_(max_attempt_times), callback_(callback), candidates_(std::move(candidates)) {}

  void onSuccess(const SendResult& send_result) override;

  void onException(const MQException& e) override;

private:
  std::weak_ptr<ClientManager> client_manager_;
  absl::flat_hash_map<std::string, std::string> metadata_;
  SendMessageRequest request_;
  int attempt_times_{0};
  int max_attempt_times_;
  SendCallback* callback_;
  std::vector<MQMessageQueue> candidates_;
};

ROCKETMQ_NAMESPACE_END