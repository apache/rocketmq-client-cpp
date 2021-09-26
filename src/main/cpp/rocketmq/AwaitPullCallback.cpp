#include "AwaitPullCallback.h"

ROCKETMQ_NAMESPACE_BEGIN

void AwaitPullCallback::onSuccess(const PullResult& pull_result) noexcept {
  absl::MutexLock lk(&mtx_);
  completed_ = true;
  // TODO: optimize out messages copy here.
  pull_result_ = pull_result;
  cv_.SignalAll();
}

void AwaitPullCallback::onFailure(const std::error_code& ec) noexcept {
  absl::MutexLock lk(&mtx_);
  completed_ = true;
  ec_ = ec;
  cv_.SignalAll();
}

bool AwaitPullCallback::await() {
  {
    absl::MutexLock lk(&mtx_);
    while (!completed_) {
      cv_.Wait(&mtx_);
    }
    return !hasFailure();
  }
}

ROCKETMQ_NAMESPACE_END