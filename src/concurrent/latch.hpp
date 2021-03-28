/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ROCKETMQ_CONCURRENT_LATCH_HPP_
#define ROCKETMQ_CONCURRENT_LATCH_HPP_

#include <cassert>
#include <cstddef>

#include <atomic>
#include <future>
#include <thread>

#include "time.hpp"

namespace rocketmq {

class latch {
 public:
  explicit latch(ptrdiff_t value)
      : start_count_(value), count_(value), promise_(), future_(promise_.get_future()), waiting_(0) {
    assert(count_ >= 0);
  }

  ~latch() {
    if (!is_ready()) {
      count_ = -1;
      cancel_wait("latch is destructed");
    }

    while (waiting_ > 0) {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  }

  latch(const latch&) = delete;
  latch& operator=(const latch&) = delete;

  void count_down_and_wait() {
    if (try_count_down(1)) {
      awake_waiter();
    } else {
      wait();
    }
  }

  void count_down(ptrdiff_t n = 1) {
    if (try_count_down(n)) {
      awake_waiter();
    }
  }

  bool is_ready() const noexcept { return count_ == 0; }

  void wait() const {
    if (count_ > 0) {
      waiting_++;
      if (count_ > 0) {
        future_.wait();
      }
      waiting_--;
    }
  }

  void wait(long timeout, time_unit unit) const {
    if (count_ > 0) {
      waiting_++;
      if (count_ > 0) {
        auto time_point = until_time_point(timeout, unit);
        future_.wait_until(time_point);
      }
      waiting_--;
    }
  }

  void reset() {
    ptrdiff_t expected = count_.load();
    if (expected == start_count_ || expected == -1) {
      return;
    }
    while (!count_.compare_exchange_strong(expected, -1)) {
      expected = count_.load();
    }
    if (expected > 0) {
      cancel_wait("reset");
    }
    while (waiting_ > 0) {  // spin
      std::this_thread::yield();
    }
    promise_ = std::promise<bool>();
    future_ = promise_.get_future();
    count_ = start_count_;
  }

 private:
  bool try_count_down(ptrdiff_t n) {
    for (;;) {
      auto c = count_.load();
      if (c <= 0) {
        return false;
      }
      auto nextc = c - n;
      if (count_.compare_exchange_weak(c, nextc)) {
        return nextc <= 0;
      }
    }
  }

  void awake_waiter() {
    try {
      promise_.set_value(true);
    } catch (...) {
    }
  }

  void cancel_wait(const char* reason) {
    try {
      promise_.set_value(false);
    } catch (...) {
    }
  }

 private:
  ptrdiff_t start_count_;
  std::atomic<ptrdiff_t> count_;
  std::promise<bool> promise_;
  std::future<bool> future_;

  mutable std::atomic<int> waiting_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONCURRENT_LATCH_HPP_
