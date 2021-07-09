#include "CountdownLatch.h"
#include "LoggerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

void CountdownLatch::await() {
  absl::MutexLock lock(&mtx_);
  if (count_ <= 0) {
    return;
  }
  while (count_ > 0) {
    cv_.Wait(&mtx_);
  }
}

void CountdownLatch::countdown() {
  absl::MutexLock lock(&mtx_);
  if (--count_ <= 0) {
    cv_.SignalAll();
  }
  if (!name_.empty()) {
    if (count_ >= 0) {
      SPDLOG_TRACE("After countdown(), latch[{}]={}", name_, count_);
    }
  }
}

void CountdownLatch::increaseCount() {
  absl::MutexLock lock(&mtx_);
  ++count_;
  if (!name_.empty()) {
    SPDLOG_TRACE("After increaseCount(), latch[{}]={}", name_, count_);
  }
}

ROCKETMQ_NAMESPACE_END