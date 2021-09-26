#pragma once

#include <system_error>

#include "absl/synchronization/mutex.h"

#include "rocketmq/AsyncCallback.h"

ROCKETMQ_NAMESPACE_BEGIN

class AwaitPullCallback : public PullCallback {
public:
  explicit AwaitPullCallback(PullResult& pull_result) : pull_result_(pull_result) {}

  void onSuccess(const PullResult& pull_result) noexcept override;

  void onFailure(const std::error_code& ec) noexcept override;

  bool await();

  bool hasFailure() const { return ec_.operator bool(); }

  bool isCompleted() const { return completed_; }

  const std::error_code& errorCode() const noexcept { return ec_; }

private:
  PullResult& pull_result_;
  absl::Mutex mtx_;
  absl::CondVar cv_;
  bool completed_{false};
  std::error_code ec_;
};

ROCKETMQ_NAMESPACE_END