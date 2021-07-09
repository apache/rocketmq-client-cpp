#include "Scheduler.h"

#include "LoggerImpl.h"
#include "absl/memory/memory.h"
#include "src/core/lib/iomgr/closure.h"
#include <cstdint>
#include <functional>

ROCKETMQ_NAMESPACE_BEGIN

void Scheduler::clear(TimerTask* timer_task) {
  absl::MutexLock lk(&tasks_mtx_);
  tasks_.erase(reinterpret_cast<std::uintptr_t>(timer_task));
}

void Scheduler::onTrigger(void* arg, grpc_error_handle error) {
  auto timer_task = reinterpret_cast<TimerTask*>(arg);
  if (GRPC_ERROR_CANCELLED == error) {
    SPDLOG_INFO("Task[{}] has been cancelled", timer_task->task_name);
    timer_task->scheduler->clear(timer_task);
    delete timer_task;
    return;
  }

  /**
   * Consider the following fact that grpc_timer_cancel treats fail-to-cancel-in-flight task as success.
   * Tasks without proper handles are deleted directly.
   * ==================================================================================================
   * Cancel an *timer.
   * There are three cases:
   * 1. We normally cancel the timer
   * 2. The timer has already run
   * 3. We can't cancel the timer because it is "in flight".
   *
   * In all of these cases, the cancellation is still considered successful.
   * They are essentially distinguished in that the timer_cb will be run
   * exactly once from either the cancellation (with error GRPC_ERROR_CANCELLED)
   * or from the activation (with error GRPC_ERROR_NONE).
   *
   * Note carefully that the callback function MAY occur in the same callstack
   * as grpc_timer_cancel. It's expected that most timers will be cancelled (their
   * primary use is to implement deadlines), and so this code is optimized such
   * that cancellation costs as little as possible. Making callbacks run inline
   * matches this aim.
   *
   * Requires: cancel() must happen after init() on a given timer.
   * ==================================================================================================
   */
  {
    absl::MutexLock lk(&timer_task->scheduler->tasks_mtx_);
    uintptr_t ptr = reinterpret_cast<uintptr_t>(timer_task);
    if (!timer_task->scheduler->tasks_.contains(ptr)) {
      SPDLOG_WARN("Task[{}] should have been cancelled", timer_task->task_name);
      delete timer_task;
      return;
    }
  }

  SPDLOG_DEBUG("Execute timer-task: {}", timer_task->task_name);
  timer_task->callback();

  if (!timer_task->interval) {
    SPDLOG_DEBUG("Completed single-shot timer task[{}]", timer_task->task_name);
    timer_task->scheduler->clear(timer_task);
    delete timer_task;
    return;
  }

  SPDLOG_DEBUG("Periodic task[name={}] is scheduled for the next round", timer_task->task_name);
  grpc_core::ExecCtx exec_ctx;
  grpc_timer_init(&timer_task->g_timer, grpc_core::ExecCtx::Get()->Now() + timer_task->interval,
                  GRPC_CLOSURE_CREATE(&Scheduler::onTrigger, timer_task, grpc_schedule_on_exec_ctx));
}

Scheduler::Scheduler() { spdlog::set_level(spdlog::level::debug); }

Scheduler::~Scheduler() {}

std::uintptr_t Scheduler::schedule(const std::function<void(void)>& functor, const std::string& task_name,
                                   std::chrono::milliseconds delay, std::chrono::milliseconds interval) {
  auto timer_task = new TimerTask();
  timer_task->callback = std::move(functor);
  timer_task->scheduler = this;
  timer_task->interval = interval.count();
  timer_task->task_name = task_name;

  {
    absl::MutexLock lk(&tasks_mtx_);
    tasks_.insert(reinterpret_cast<std::uintptr_t>(timer_task));
  }

  grpc_core::ExecCtx exec_ctx;
  grpc_timer_init(&timer_task->g_timer, grpc_core::ExecCtx::Get()->Now() + delay.count(),
                  GRPC_CLOSURE_CREATE(&Scheduler::onTrigger, timer_task, grpc_schedule_on_exec_ctx));
  SPDLOG_DEBUG("Task[name={}] scheduled OK", timer_task->task_name);
  return reinterpret_cast<std::uintptr_t>(timer_task);
}

void Scheduler::cancel(std::uintptr_t task_id) {
  {
    absl::MutexLock lk(&tasks_mtx_);
    if (!tasks_.contains(task_id)) {
      SPDLOG_WARN("No matched schedule task found. Cancel ignored");
      return;
    }
  }

  auto timer_task = reinterpret_cast<TimerTask*>(task_id);
  grpc_core::ExecCtx exec_ctx;
  grpc_timer_cancel(&timer_task->g_timer);
  SPDLOG_DEBUG("Scheduled task[name={}] will get cancelled", timer_task->task_name);

  clear(timer_task);
}

ROCKETMQ_NAMESPACE_END