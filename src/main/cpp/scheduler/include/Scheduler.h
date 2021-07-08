#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "CountdownLatch.h"
#include "Functional.h"
#include "LoggerImpl.h"
#include "ThreadPool.h"
#include "uv.h"

namespace rocketmq {
class Scheduler {
  /**
   * uv_handle_t attachment structure.
   */
  enum TaskType { TIMER, CANCEL };

  template <typename Duration> struct Task {
    std::weak_ptr<Functional> functor_ptr_;
    Duration delay_;
    Duration interval_;
    TaskType type_;

    // pointer to cancelled task
    void* cancelled_callable_;
    Task(std::weak_ptr<Functional> functor_ptr, Duration delay, Duration interval, TaskType type = TIMER)
        : functor_ptr_(std::move(functor_ptr)), delay_(delay), interval_(interval), type_(type),
          cancelled_callable_(nullptr) {}
  };

  struct Attachment {
    TaskType type_;
    uv_handle_t* uv_handle_;
    explicit Attachment(TaskType type, uv_handle_t* uv_handle) : type_(type), uv_handle_(uv_handle) {}

    virtual ~Attachment() {
      if (TIMER == type_) {
        delete reinterpret_cast<uv_timer_t*>(uv_handle_);
      } else {
        delete reinterpret_cast<uv_async_t*>(uv_handle_);
      }
    }
  };

  struct Timer : public Attachment {
    FunctionalWeakPtr callback_;
    Scheduler* scheduler_;
    Timer(uv_timer_t* timer_handle, FunctionalWeakPtr callback, Scheduler* scheduler)
        : Attachment(TIMER, reinterpret_cast<uv_handle_t*>(timer_handle)), callback_(std::move(callback)),
          scheduler_(scheduler) {}

    ~Timer() override {
      SPDLOG_DEBUG("~Timer invoked");
    }
  };

public:
  Scheduler();

  ~Scheduler();

  void start();

  void shutdown();

  /**
   * @callable Pointer to the functor. Lifecycle of this functor should be maintained by the caller.
   * @delay The amount of time to wait before the first shot.
   * @interval The interval between each fire-shot. If it is 0ms, callable will just fired once.
   * @fixed_delay Execute callable with fixed delay if true; otherwise, scheduler will try its best to execute callable
   * with fixed rate
   */
  bool schedule(const FunctionalWeakPtr& functor, std::chrono::milliseconds delay, std::chrono::milliseconds interval)
      LOCKS_EXCLUDED(pending_task_list_mtx_);

  bool cancel(std::function<void(void)>* callable) LOCKS_EXCLUDED(pending_task_list_mtx_);

private:
  uv_loop_t loop_;

  /**
   * Used to notify loop thread that some external events are pending to process
   */
  uv_async_t async_handle_;

  std::atomic_bool running_;

  std::thread loop_thread_;

  CountdownLatch uv_loop_latch_;

  std::function<void(void)> daemon_callback_;
  FunctionalSharePtr functor_;

  std::vector<Task<std::chrono::milliseconds>> pending_task_list_ GUARDED_BY(pending_task_list_mtx_);
  absl::Mutex pending_task_list_mtx_; // Protects pending_task_list_.

  std::unique_ptr<ThreadPool> thread_pool_;

  static void uv_timer_callback(uv_timer_t* handle);

  static void uv_close_callback(uv_handle_t* handle);

  static void uv_async_do_schedule(uv_async_t* handle);

  static void uv_queue_work_callback(uv_work_t* req);

  void do_daemon();

  /**
   * Will execute in uv_loop thread.
   */
  void do_schedule();

  static void uv_stop_walk_cb(uv_handle_t* handle, void* arg);

  static void uv_cancel_walk_cb(uv_handle_t* handle, void* arg);

  void check_and_cancel(uv_handle_t* handle) LOCKS_EXCLUDED(pending_task_list_mtx_);

  static std::string getThreadId();
};
} // namespace rocketmq