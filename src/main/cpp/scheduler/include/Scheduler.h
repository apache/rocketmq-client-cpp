#pragma once
#include "absl/base/thread_annotations.h"
#include "absl/container/flat_hash_set.h"
#include "absl/synchronization/mutex.h"
#include "rocketmq/RocketMQ.h"
#include "src/core/lib/iomgr/timer.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

ROCKETMQ_NAMESPACE_BEGIN

class Scheduler;

struct TimerTask {
  Scheduler* scheduler;
  grpc_timer g_timer;
  std::function<void(void)> callback;
  grpc_millis interval;
  std::string task_name;
};

class Scheduler {
public:
  Scheduler();

  ~Scheduler();

  /**
   * @functor Pointer to the functor. Lifecycle of this functor should be maintained by the caller.
   * @task_name Name of the task. Task name would be very helpful at debug site.
   * @delay The amount of time to wait before the first shot.
   * @interval The interval between each fire-shot. If it is 0ms, callable will just fired once.
   */
  std::uintptr_t schedule(const std::function<void(void)>& functor, const std::string& task_name,
                          std::chrono::milliseconds delay, std::chrono::milliseconds interval)
      LOCKS_EXCLUDED(tasks_mtx_);

  /**
   * Note:
   * Periodic tasks should be explicitly cancelled once they are no longer needed.
   */
  void cancel(std::uintptr_t task_id) LOCKS_EXCLUDED(tasks_mtx_);

private:
  static void onTrigger(void* arg, grpc_error_handle error);

  void clear(TimerTask* timer_task) LOCKS_EXCLUDED(tasks_mtx_);

  absl::flat_hash_set<std::uintptr_t> tasks_ GUARDED_BY(tasks_mtx_);
  absl::Mutex tasks_mtx_;
};

ROCKETMQ_NAMESPACE_END