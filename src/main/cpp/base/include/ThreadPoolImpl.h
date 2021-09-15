#pragma once

#include "ThreadPool.h"
#include "absl/synchronization/mutex.h"
#include "asio/io_context.hpp"
#include "rocketmq/RocketMQ.h"
#include "rocketmq/State.h"
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "asio.hpp"

ROCKETMQ_NAMESPACE_BEGIN

class ThreadPoolImpl : public ThreadPool {
public:
  ThreadPoolImpl(std::uint16_t workers);

  ~ThreadPoolImpl() override = default;

  void start() override;

  void shutdown() override;

  void submit(std::function<void(void)> task) override;

private:
  asio::io_context context_;
  std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> work_guard_;
  std::uint16_t workers_;
  std::vector<std::thread> threads_;
  std::atomic<State> state_{State::CREATED};
  absl::Mutex start_mtx_;
  absl::CondVar start_cv_;
};

ROCKETMQ_NAMESPACE_END