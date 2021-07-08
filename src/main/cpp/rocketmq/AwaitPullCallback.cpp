#include "AwaitPullCallback.h"

ROCKETMQ_NAMESPACE_BEGIN

void AwaitPullCallback::onSuccess(const PullResult& pull_result) {
  absl::MutexLock lk(&mtx_);
  has_failure_ = false;
  completed_ = true;
  // TODO: optimize out messages copy here.
  pull_result_ = pull_result;
  cv_.SignalAll();
}

void AwaitPullCallback::onException(const MQException& e) {
  absl::MutexLock lk(&mtx_);
  has_failure_ = true;
  completed_ = true;
  cv_.SignalAll();
}

bool AwaitPullCallback::await() {
  {
    absl::MutexLock lk(&mtx_);
    while (!completed_) {
      cv_.Wait(&mtx_);
    }
    return !has_failure_;
  }
}

ROCKETMQ_NAMESPACE_END