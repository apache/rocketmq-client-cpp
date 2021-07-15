#include "Semaphore.h"

using namespace rocketmq;

void Semaphore::acquire() {
  absl::MutexLock lock(&mtx_);
  if (value_ > 0) {
    value_--;
    return;
  } else {
    while (value_ <= 0) {
      cv_.Wait(&mtx_);
    }
    value_--;
    return;
  }
}

void Semaphore::release() {
  {
    absl::MutexLock lock(&mtx_);
    value_++;
  }
  cv_.Signal();
}
