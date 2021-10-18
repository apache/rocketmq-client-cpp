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
#include "SchedulerImpl.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <system_error>
#include <thread>

#include "absl/memory/memory.h"
#include "asio/error_code.hpp"
#include "asio/executor_work_guard.hpp"
#include "asio/io_context.hpp"
#include "asio/steady_timer.hpp"

#include "LoggerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

SchedulerImpl::SchedulerImpl()
    : work_guard_(
          absl::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(context_.get_executor())) {
  spdlog::set_level(spdlog::level::debug);
}

void SchedulerImpl::start() {
  State expected = State::CREATED;
  if (state_.compare_exchange_strong(expected, State::STARTING, std::memory_order_relaxed)) {

    for (unsigned int i = 0; i < std::thread::hardware_concurrency(); i++) {
      auto worker = std::thread([this]() {
        {
          State expect = State::STARTING;
          if (state_.compare_exchange_strong(expect, State::STARTED, std::memory_order_relaxed)) {
            absl::MutexLock lk(&start_mtx_);
            start_cv_.SignalAll();
          }
        }

        while (true) {
#ifdef __EXCEPTIONS
          try {
#endif
            std::error_code ec;
            context_.run(ec);
            if (ec) {
              SPDLOG_WARN("Error raised from thread-pool: {}", ec.message());
            }
#ifdef __EXCEPTIONS
          } catch (std::exception& e) {
            SPDLOG_WARN("Exception raised from thread-pool: {}", e.what());
          }
#endif

          if (State::STARTED != state_.load(std::memory_order_relaxed)) {
            SPDLOG_INFO("A scheduler worker quit");
            break;
          }
        }
      });
      threads_.emplace_back(std::move(worker));
    }

    {
      absl::MutexLock lk(&start_mtx_);
      if (State::STARTING == state_.load(std::memory_order_relaxed)) {
        start_cv_.Wait(&start_mtx_);
        SPDLOG_INFO("Scheduler threads start to loop");
      }
    }
  }
}

void SchedulerImpl::shutdown() {
  State expected = State::STARTED;
  if (state_.compare_exchange_strong(expected, State::STOPPING, std::memory_order_relaxed)) {
    work_guard_->reset();
    {
      absl::MutexLock lk(&tasks_mtx_);
      tasks_.clear();
    }
    context_.stop();

    for (auto& worker : threads_) {
      if (worker.joinable()) {
        worker.join();
      }
    }

    state_.store(State::STOPPED);
  }
}

std::uint32_t SchedulerImpl::schedule(const std::function<void(void)>& functor, const std::string& task_name,
                                      std::chrono::milliseconds delay, std::chrono::milliseconds interval) {
  static std::uint32_t task_id = 0;

  auto task = std::make_shared<TimerTask>();
  task->task_name = task_name;
  task->callback = functor;
  task->interval = interval;

  std::uint32_t id;
  {
    absl::MutexLock lk(&tasks_mtx_);
    id = ++task_id;
    tasks_.insert({id, task});
  }
  asio::steady_timer* timer = new asio::steady_timer(context_, delay);
  SPDLOG_DEBUG("Timer-task[name={}] to fire in {}ms", task_name, delay.count());
  timer->async_wait(std::bind(&SchedulerImpl::execute, std::placeholders::_1, timer, std::weak_ptr<TimerTask>(task)));
  return id;
}

void SchedulerImpl::cancel(std::uint32_t task_id) {
  absl::MutexLock lk(&tasks_mtx_);
  if (!tasks_.contains(task_id)) {
    SPDLOG_ERROR("Scheduler does not have the task to delete. Task-ID specified is: {}", task_id);
    return;
  }
  auto search = tasks_.find(task_id);
  assert(search != tasks_.end());
  SPDLOG_INFO("Cancel task[task-id={}, name={}]", task_id, search->second->task_name);
  tasks_.erase(search);
}

void SchedulerImpl::execute(const asio::error_code& ec, asio::steady_timer* timer, std::weak_ptr<TimerTask> task) {
  std::shared_ptr<TimerTask> timer_task = task.lock();
  if (!timer_task) {
    delete timer;
    return;
  }

  SPDLOG_INFO("Execute task: {}. Use-count: {}", timer_task->task_name, timer_task.use_count());

  // Execute the actual callback.
#ifdef __EXCEPTIONS
  try {
#endif
    timer_task->callback();
#ifdef __EXCEPTIONS
  } catch (std::exception& e) {
    SPDLOG_WARN("Exception raised: {}", e.what());
  } catch (std::string& e) {
    SPDLOG_WARN("Exception raised: {}", e);
  } catch (...) {
    SPDLOG_WARN("Unknown exception type raised");
  }
#endif

  if (timer_task->interval.count()) {
    timer->expires_at(timer->expiry() + timer_task->interval);
    timer->async_wait(std::bind(&SchedulerImpl::execute, std::placeholders::_1, timer, task));
    SPDLOG_DEBUG("Repeated timer-task {} to fire in {}ms", timer_task->task_name, timer_task->interval.count());
  } else {
    delete timer;
  }
}

ROCKETMQ_NAMESPACE_END