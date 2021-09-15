#include "ThreadPoolImpl.h"
#include "absl/memory/memory.h"
#include "asio/executor_work_guard.hpp"
#include "asio/io_context.hpp"
#include "asio/post.hpp"
#include "rocketmq/RocketMQ.h"
#include "rocketmq/State.h"
#include "spdlog/spdlog.h"
#include <atomic>
#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

ThreadPoolImpl::ThreadPoolImpl(std::uint16_t workers)
    : work_guard_(
          absl::make_unique<asio::executor_work_guard<asio::io_context::executor_type>>(context_.get_executor())),
      workers_(workers) {}

void ThreadPoolImpl::start() {
  for (std::uint16_t i = 0; i < workers_; i++) {
    std::thread worker([this]() {
      State expected = State::CREATED;
      if (state_.compare_exchange_strong(expected, State::STARTED, std::memory_order_relaxed)) {
        absl::MutexLock lk(&start_mtx_);
        start_cv_.SignalAll();
      }
      context_.run();
    });
    threads_.emplace_back(std::move(worker));
  }

  {
    absl::MutexLock lk(&start_mtx_);
    if (State::CREATED == state_.load(std::memory_order_relaxed)) {
      start_cv_.Wait(&start_mtx_);
    }
  }
}

void ThreadPoolImpl::shutdown() {
  State expected = State::STARTED;
  if (state_.compare_exchange_strong(expected, State::STOPPING, std::memory_order_relaxed)) {
    work_guard_->reset();
    context_.stop();
    for (auto& thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
    state_.store(State::STOPPED, std::memory_order_relaxed);
  }
}

void ThreadPoolImpl::submit(std::function<void()> task) {
  if (State::STARTED == state_.load(std::memory_order_relaxed)) {
    asio::post(context_, task);
  } else {
    SPDLOG_WARN("State of ThreadPool is not STARTED");
  }
}

ROCKETMQ_NAMESPACE_END