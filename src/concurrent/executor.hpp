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
#ifndef ROCKETMQ_CONCURRENT_EXECUTOR_HPP_
#define ROCKETMQ_CONCURRENT_EXECUTOR_HPP_

#include <functional>
#include <future>
#include <utility>

#include "time.hpp"

namespace rocketmq {

typedef std::function<void()> handler_type;

struct executor_handler {
  const handler_type handler_;
  std::unique_ptr<std::promise<void>> promise_;

  template <typename H,
            typename std::enable_if<std::is_same<typename std::decay<H>::type, handler_type>::value, int>::type = 0>
  explicit executor_handler(H&& handler)
      : handler_(std::forward<handler_type>(handler)), promise_(new std::promise<void>) {}

  void operator()() noexcept {
    // call handler, then set promise
    try {
      // handler that may throw
      handler_();
      promise_->set_value();
    } catch (...) {
      try {
        // store anything thrown in the promise
        promise_->set_exception(std::current_exception());
      } catch (...) {
      }  // set_exception() may throw too
    }
  }

  template <class E>
  void abort(E&& exception) noexcept {
    promise_->set_exception(std::make_exception_ptr(std::forward<E>(exception)));
  }

 private:
  executor_handler() = delete;
};

class executor {
 public:
  virtual ~executor() = default;

  virtual void execute(std::unique_ptr<executor_handler> command) = 0;
};

class executor_service : virtual public executor {
 public:
  virtual void shutdown(bool immediately) = 0;
  virtual bool is_shutdown() = 0;
  virtual std::future<void> submit(const handler_type& task) = 0;
};

class scheduled_executor_service : virtual public executor_service {
 public:
  virtual std::future<void> schedule(const handler_type& task, long delay, time_unit unit) = 0;
};

}  // namespace rocketmq

#include "executor_impl.hpp"

#endif  // ROCKETMQ_CONCURRENT_EXECUTOR_HPP_
