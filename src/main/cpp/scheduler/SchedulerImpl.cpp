#include "SchedulerImpl.h"

#include "LoggerImpl.h"
#include "absl/memory/memory.h"
#include "asio/error_code.hpp"
#include "asio/executor_work_guard.hpp"
#include "asio/io_context.hpp"
#include "asio/steady_timer.hpp"
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <thread>

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
        context_.run();
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
  timer_task->callback();

  if (timer_task->interval.count()) {
    timer->expires_at(timer->expiry() + timer_task->interval);
    timer->async_wait(std::bind(&SchedulerImpl::execute, std::placeholders::_1, timer, task));
    SPDLOG_DEBUG("Repeated timer-task to fire in {}ms", timer_task->interval.count());
  } else {
    delete timer;
  }
}

ROCKETMQ_NAMESPACE_END