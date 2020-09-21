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
#ifndef ROCKETMQ_CONCURRENT_EXECUTORIMPL_HPP_
#define ROCKETMQ_CONCURRENT_EXECUTORIMPL_HPP_

#include <atomic>
#include <functional>
#include <memory>
#include <queue>

#include "concurrent_queue.hpp"
#include "thread_group.hpp"
#include "time.hpp"

namespace rocketmq {

class abstract_executor_service : virtual public executor_service {
 public:
  std::future<void> submit(const handler_type& task) override {
    std::unique_ptr<executor_handler> handler(new executor_handler(const_cast<handler_type&>(task)));
    std::future<void> fut = handler->promise_->get_future();
    execute(std::move(handler));
    return fut;
  }
};

class thread_pool_executor : public abstract_executor_service {
 public:
  explicit thread_pool_executor(std::size_t thread_nums, bool start_immediately = true)
      : task_queue_(), state_(STOP), thread_nums_(thread_nums), free_threads_(0) {
    if (start_immediately) {
      startup();
    }
  }
  explicit thread_pool_executor(const std::string& name, std::size_t thread_nums, bool start_immediately = true)
      : task_queue_(), state_(STOP), thread_nums_(thread_nums), thread_group_(name), free_threads_(0) {
    if (start_immediately) {
      startup();
    }
  }

  virtual void startup() {
    if (state_ == STOP) {
      state_ = RUNNING;
      thread_group_.create_threads(std::bind(&thread_pool_executor::run, this), thread_nums_);
      thread_group_.start();
    }
  }

  void shutdown(bool immediately = true) override {
    if (state_ == RUNNING) {
      state_ = immediately ? STOP : SHUTDOWN;
      wakeup_event_.notify_all();
      thread_group_.join();
      state_ = STOP;
    }
  }

  bool is_shutdown() override { return state_ != RUNNING; }

  std::size_t thread_nums() { return thread_nums_; }
  void set_thread_nums(std::size_t thread_nums) { thread_nums_ = thread_nums; }

 protected:
  static const unsigned int ACCEPT_NEW_TASKS = 1U << 0U;
  static const unsigned int PROCESS_QUEUED_TASKS = 1U << 1U;

  enum state { STOP = 0, SHUTDOWN = PROCESS_QUEUED_TASKS, RUNNING = ACCEPT_NEW_TASKS | PROCESS_QUEUED_TASKS };

  void execute(std::unique_ptr<executor_handler> command) override {
    if (state_ & ACCEPT_NEW_TASKS) {
      task_queue_.push_back(command.release());
      if (free_threads_ > 0) {
        wakeup_event_.notify_one();
      }
    } else {
      command->abort(std::logic_error("executor don't accept new tasks."));
    }
  }

  void run() {
    while (state_ & PROCESS_QUEUED_TASKS) {
      auto task = task_queue_.pop_front();
      if (task != nullptr) {
        task->operator()();
      } else {
        if (!(state_ & ACCEPT_NEW_TASKS)) {
          // don't accept new tasks
          break;
        }

        // wait new tasks
        std::unique_lock<std::mutex> lock(wakeup_mutex_);
        if (task_queue_.empty()) {
          free_threads_++;
          wakeup_event_.wait_for(lock, std::chrono::seconds(5));
          free_threads_--;
        }
      }
    }
  }

 protected:
  concurrent_queue<executor_handler> task_queue_;

 private:
  state state_;

  std::size_t thread_nums_;
  thread_group thread_group_;

  std::mutex wakeup_mutex_;
  std::condition_variable wakeup_event_;
  std::atomic<int> free_threads_;
};

struct scheduled_executor_handler : public executor_handler {
  std::chrono::steady_clock::time_point wakeup_time_;

  template <typename H,
            typename std::enable_if<std::is_same<typename std::decay<H>::type, handler_type>::value, int>::type = 0>
  explicit scheduled_executor_handler(H&& handler, const std::chrono::steady_clock::time_point& time)
      : executor_handler(std::forward<handler_type>(handler)), wakeup_time_(time) {}

  bool operator<(const scheduled_executor_handler& other) const { return (wakeup_time_ > other.wakeup_time_); }

  static bool less(const std::unique_ptr<scheduled_executor_handler>& a,
                   const std::unique_ptr<scheduled_executor_handler>& b) {
    return *a < *b;
  }
};

class scheduled_thread_pool_executor : public thread_pool_executor, virtual public scheduled_executor_service {
 public:
  explicit scheduled_thread_pool_executor(std::size_t thread_nums, bool start_immediately = true)
      : thread_pool_executor(thread_nums, false),
        time_queue_(&scheduled_executor_handler::less),
        stopped_(true),
        single_thread_(false),
        timer_thread_() {
    timer_thread_.set_target(&scheduled_thread_pool_executor::time_daemon, this);
    if (start_immediately) {
      startup();
    }
  }
  explicit scheduled_thread_pool_executor(const std::string& name,
                                          std::size_t thread_nums,
                                          bool start_immediately = true)
      : thread_pool_executor(name, thread_nums, false),
        time_queue_(&scheduled_executor_handler::less),
        stopped_(true),
        single_thread_(false),
        timer_thread_(name + "-Timer") {
    timer_thread_.set_target(&scheduled_thread_pool_executor::time_daemon, this);
    if (start_immediately) {
      startup();
    }
  }

  explicit scheduled_thread_pool_executor(bool start_immediately = true)
      : thread_pool_executor(0, false),
        time_queue_(&scheduled_executor_handler::less),
        stopped_(true),
        single_thread_(true),
        timer_thread_() {
    timer_thread_.set_target(&scheduled_thread_pool_executor::time_daemon, this);
    if (start_immediately) {
      startup();
    }
  }
  explicit scheduled_thread_pool_executor(const std::string& name, bool start_immediately = true)
      : thread_pool_executor(name, 0, false),
        time_queue_(&scheduled_executor_handler::less),
        stopped_(true),
        single_thread_(true),
        timer_thread_(name + "-Timer") {
    timer_thread_.set_target(&scheduled_thread_pool_executor::time_daemon, this);
    if (start_immediately) {
      startup();
    }
  }

  void startup() override {
    if (stopped_) {
      stopped_ = false;

      // startup task threads
      if (!single_thread_) {
        thread_pool_executor::startup();
      }

      // start time daemon
      timer_thread_.start();
    }
  }

  void shutdown(bool immediately = true) override {
    if (!stopped_) {
      stopped_ = true;

      time_event_.notify_one();
      timer_thread_.join();

      if (!single_thread_) {
        thread_pool_executor::shutdown(immediately);
      }
    }
  }

  bool is_shutdown() override { return stopped_; }

  std::future<void> submit(const handler_type& task) override {
    if (!single_thread_) {
      return thread_pool_executor::submit(task);
    } else {
      return schedule(task, 0, time_unit::milliseconds);
    }
  }

  std::future<void> schedule(const handler_type& task, long delay, time_unit unit) override {
    auto time_point = until_time_point(delay, unit);
    std::unique_ptr<scheduled_executor_handler> handler(
        new scheduled_executor_handler(const_cast<handler_type&>(task), time_point));
    std::future<void> fut = handler->promise_->get_future();

    {
      std::unique_lock<std::mutex> lock(time_mutex_);
      if (time_queue_.empty() || time_queue_.top()->wakeup_time_ < time_point) {
        time_queue_.push(std::move(handler));
        time_event_.notify_one();
      } else {
        time_queue_.push(std::move(handler));
      }
    }

    return fut;
  }

 protected:
  void time_daemon() {
    std::unique_lock<std::mutex> lock(time_mutex_);
    while (!stopped_) {
      auto now = std::chrono::steady_clock::now();
      while (!time_queue_.empty()) {
        auto& top = const_cast<std::unique_ptr<scheduled_executor_handler>&>(time_queue_.top());
        if (top->wakeup_time_ <= now) {
          if (!single_thread_) {
            thread_pool_executor::execute(std::move(top));
            time_queue_.pop();
          } else {
            auto copy = std::move(top);
            time_queue_.pop();
            lock.unlock();
            (*copy)();
            lock.lock();

            // if function cost more time, we need re-watch clock
            now = std::chrono::steady_clock::now();
          }
        } else {
          break;
        }
      }

      if (!time_queue_.empty()) {
        const auto& top = time_queue_.top();
        // wait more 10 milliseconds
        time_event_.wait_for(lock, top->wakeup_time_ - now + std::chrono::milliseconds(10));
      } else {
        // default, wakeup after 10 seconds for check stopped flag.
        time_event_.wait_for(lock, std::chrono::seconds(10));
      }
    }
  }

 protected:
  std::priority_queue<std::unique_ptr<scheduled_executor_handler>,
                      std::vector<std::unique_ptr<scheduled_executor_handler>>,
                      std::function<bool(const std::unique_ptr<scheduled_executor_handler>&,
                                         const std::unique_ptr<scheduled_executor_handler>&)>>
      time_queue_;

 private:
  bool stopped_;

  bool single_thread_;
  thread timer_thread_;

  std::mutex time_mutex_;
  std::condition_variable time_event_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONCURRENT_EXECUTORIMPL_HPP_
