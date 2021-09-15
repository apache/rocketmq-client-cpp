#pragma once
#include "Scheduler.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_map.h"
#include "absl/synchronization/mutex.h"
#include "asio.hpp"
#include "rocketmq/State.h"

ROCKETMQ_NAMESPACE_BEGIN

struct TimerTask {
  std::function<void(void)> callback;
  std::chrono::milliseconds interval;
  std::string task_name;
};

class SchedulerImpl : public Scheduler {
public:
  SchedulerImpl();

  ~SchedulerImpl() override = default;

  void start() override;

  void shutdown() override LOCKS_EXCLUDED(tasks_mtx_);

  /**
   * @functor Pointer to the functor. Lifecycle of this functor should be maintained by the caller.
   * @task_name Name of the task. Task name would be very helpful at debug site.
   * @delay The amount of time to wait before the first shot.
   * @interval The interval between each fire-shot. If it is 0ms, callable will just fired once.
   */
  std::uint32_t schedule(const std::function<void(void)>& functor, const std::string& task_name,
                         std::chrono::milliseconds delay, std::chrono::milliseconds interval) override
      LOCKS_EXCLUDED(tasks_mtx_);

  /**
   * Note:
   * Periodic tasks should be explicitly cancelled once they are no longer needed.
   */
  void cancel(std::uint32_t task_id) override LOCKS_EXCLUDED(tasks_mtx_);

private:
  asio::io_context context_;
  std::unique_ptr<asio::executor_work_guard<asio::io_context::executor_type>> work_guard_;
  absl::Mutex start_mtx_;
  absl::CondVar start_cv_;
  std::vector<std::thread> threads_;
  std::atomic<State> state_{State::CREATED};

  absl::flat_hash_map<std::uint32_t, std::shared_ptr<TimerTask>> tasks_ GUARDED_BY(tasks_mtx_);
  absl::Mutex tasks_mtx_;

  static void execute(const asio::error_code& ec, asio::steady_timer* timer, std::weak_ptr<TimerTask> task);
};

ROCKETMQ_NAMESPACE_END