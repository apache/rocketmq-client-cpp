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
#ifndef ROCKETMQ_CONCURRENT_BLOCKINGQUEUE_HPP_
#define ROCKETMQ_CONCURRENT_BLOCKINGQUEUE_HPP_

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>

#include "time.hpp"

namespace rocketmq {

template <typename T>
class blocking_queue {
 public:
  // types:
  typedef T value_type;

  virtual ~blocking_queue() = default;

  bool empty() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
  }

  size_t size() {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

  template <typename E,
            typename std::enable_if<std::is_same<typename std::decay<E>::type, value_type>::value, int>::type = 0>
  void push_back(E&& v) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.emplace_back(new value_type(std::forward<E>(v)));
    cv_.notify_one();
  }

  template <class E, typename std::enable_if<std::is_convertible<E, value_type*>::value, int>::type = 0>
  void push_back(E v) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.emplace_back(v);
    cv_.notify_one();
  }

  std::unique_ptr<value_type> pop_front() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      cv_.wait(lock, [&] { return !queue_.empty(); });
    }
    auto v = std::move(queue_.front());
    queue_.pop_front();
    return v;
  }

  std::unique_ptr<value_type> pop_front(long timeout, time_unit unit) {
    auto deadline = until_time_point(timeout, unit);
    std::unique_lock<std::mutex> lock(mutex_);
    if (queue_.empty()) {
      cv_.wait_until(lock, deadline, [&] { return !queue_.empty(); });
    }
    if (!queue_.empty()) {
      auto v = std::move(queue_.front());
      queue_.pop_front();
      return v;
    }
    return std::unique_ptr<value_type>();
  }

 private:
  std::deque<std::unique_ptr<value_type>> queue_;
  std::mutex mutex_;
  std::condition_variable cv_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONCURRENT_BLOCKINGQUEUE_HPP_
