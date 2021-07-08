#pragma once

#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"

namespace rocketmq {
class Semaphore {
public:
  Semaphore(int value) : value_(value) {}

  void acquire();

  void release();

private:
  int value_;
  absl::Mutex mtx_;
  absl::CondVar cv_;
};
} // namespace rocketmq
