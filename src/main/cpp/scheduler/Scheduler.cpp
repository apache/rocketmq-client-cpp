#include "Scheduler.h"

#include "absl/memory/memory.h"
#include <functional>
#include <iostream>
#include <sstream>

using namespace rocketmq;

Scheduler::Scheduler()
    : loop_(), async_handle_(), running_(false), uv_loop_latch_(1),
      thread_pool_(absl::make_unique<ThreadPool>(1)) {
  daemon_callback_ = std::bind(&Scheduler::do_daemon, this);
  functor_ = std::make_shared<Functional>(&daemon_callback_);
  spdlog::set_level(spdlog::level::trace);
}

Scheduler::~Scheduler() {
  if (running_.load()) {
    SPDLOG_WARN("shutdown() should have been invoked");
  }
}

void Scheduler::start() {
  bool expected = false;
  if (running_.compare_exchange_strong(expected, true)) {
    // Initialize uv_loop
    if (uv_loop_init(&loop_)) {
      SPDLOG_ERROR("uv_loop_init failed");
    }

    async_handle_.data = this;
    uv_async_init(&loop_, &async_handle_, &Scheduler::uv_async_do_schedule);

    // Initialize a daemon timer task which keeps uv_loop looping.
    auto timer_handler = new uv_timer_t();
    auto timer = new Timer(timer_handler, functor_, this);
    timer_handler->data = timer;
    uv_timer_init(&loop_, timer_handler);
    uv_timer_start(timer_handler, &Scheduler::uv_timer_callback, 0, 1000);

    // Event loop lambda
    auto loop_lambda = [this] {
      spdlog::info("uv_run starts to loop");
      if (uv_run(&loop_, UV_RUN_DEFAULT)) {
        spdlog::debug("uv_stop() was called while there are still active handles or requests.");
      } else {
        spdlog::info("uv_run completes");
      }
    };

    // Run event loop
    loop_thread_ = std::thread(loop_lambda);

    // Wait till uv_run actually fires
    uv_loop_latch_.await();
    SPDLOG_DEBUG("There are {} active handle", uv_loop_alive(&loop_));
  }
}

void Scheduler::shutdown() {
  // Mark running flag as false
  bool expected = true;
  if (running_.compare_exchange_strong(expected, false)) {
    SPDLOG_TRACE("Scheduler starts to shut down");
    uv_async_send(&async_handle_);

    // Join to wait the thread to quit
    SPDLOG_TRACE("Scheduler starts to wait loop_thread");
    if (loop_thread_.joinable()) {
      loop_thread_.join();
    }
    SPDLOG_TRACE("Scheduler awaits loop_thread OK");

    // Close uv_loop
    if (uv_loop_alive(&loop_)) {
      SPDLOG_WARN("There are {} active events", uv_loop_alive(&loop_));
    } else {
      SPDLOG_DEBUG("There is no alive uv handles any more");
    }

    int loop_close_status = uv_loop_close(&loop_);
    assert(0 == loop_close_status);
    if (loop_close_status == UV_EBUSY) {
      SPDLOG_ERROR("uv_loop_close failed with UV_EBUSY.");
      abort();
    }
    SPDLOG_DEBUG("Scheduler has shut down");
  }
}

bool Scheduler::schedule(const FunctionalWeakPtr& functor, std::chrono::milliseconds delay,
                         std::chrono::milliseconds interval) {
  if (!running_.load()) {
    return false;
  }

  if (!functor.lock()) {
    SPDLOG_DEBUG("Functor pointed by weak_ptr should have been released");
    return false;
  }

  {
    absl::MutexLock lock(&pending_task_list_mtx_);
    pending_task_list_.emplace_back(functor, delay, interval);
  }
  return 0 == uv_async_send(&async_handle_);
}

bool Scheduler::cancel(std::function<void(void)>* callable) {
  {
    absl::MutexLock lk(&pending_task_list_mtx_);
    std::weak_ptr<Functional> ptr;
    Task<std::chrono::milliseconds> task(ptr, std::chrono::milliseconds(0), std::chrono::milliseconds(0),
                                         TaskType::CANCEL);
    task.cancelled_callable_ = callable;
    pending_task_list_.push_back(task);
  }
  uv_async_send(&async_handle_);
  return true;
}

void Scheduler::uv_timer_callback(uv_timer_t* handler) {
  if (handler->data) {
    auto timer = static_cast<Timer*>(handler->data);

    auto functor_shared_ptr = timer->callback_.lock();
    if (functor_shared_ptr) {
      auto req = new uv_work_t;
      req->data = timer;
      timer->scheduler_->thread_pool_->enqueue(&Scheduler::uv_queue_work_callback, req);
    }
  }
}

void Scheduler::uv_close_callback(uv_handle_t* handle) {
  if (handle->type == uv_handle_type::UV_TIMER) {
    auto attachment = reinterpret_cast<Attachment*>(handle->data);
    // handle wil be deleted while releasing attachment
    delete attachment;
  }
  SPDLOG_DEBUG("Scheduler#uv_close_callback: a uv_handle was closed OK. ThreadId={}", getThreadId());
}

void Scheduler::uv_async_do_schedule(uv_async_t* handle) {
  auto scheduler = reinterpret_cast<Scheduler*>(handle->data);
  scheduler->do_schedule();
}

void Scheduler::do_schedule() {
  if (running_.load(std::memory_order_relaxed)) {

    // Process cancelled job
    bool has_job_cancelled = false;
    // Handle new scheduled jobs
    {
      absl::MutexLock lk(&pending_task_list_mtx_);
      for (auto it = pending_task_list_.begin(); it != pending_task_list_.end();) {
        if (it->type_ == TaskType::TIMER) {
          auto timer_handle = new uv_timer_t();
          uv_timer_init(&loop_, timer_handle);
          auto delay = it->delay_;
          auto duration = it->interval_;
          auto timer = new Timer(timer_handle, it->functor_ptr_, this);
          // Make handle#data point to the actual functor
          timer_handle->data = timer;
          if (uv_timer_start(timer_handle, &Scheduler::uv_timer_callback, delay.count(), duration.count())) {
            // As it failed to arm a timer, we just delete the resources.
            delete timer;
            timer_handle->data = nullptr;
            SPDLOG_ERROR("uv_timer_start failed");
          }
          it = pending_task_list_.erase(it);
        } else {
          if (it->type_ == TaskType::CANCEL) {
            has_job_cancelled = true;
          }
          it++;
        }
      }
    }

    if (has_job_cancelled) {
      uv_walk(&loop_, uv_cancel_walk_cb, this);
    }
  } else {
    uv_walk(&loop_, &Scheduler::uv_stop_walk_cb, nullptr);
  }
  SPDLOG_DEBUG("do_schedule threadId={}", getThreadId());
}

void Scheduler::uv_stop_walk_cb(uv_handle_t* handle, void* arg) {
  if (handle->type == uv_handle_type::UV_TIMER) {
    auto timer_handle = reinterpret_cast<uv_timer_t*>(handle);
    SPDLOG_DEBUG("uv_timer_stop a timer handle");
    uv_timer_stop(timer_handle);
  }
  uv_close(handle, &Scheduler::uv_close_callback);
}

void Scheduler::uv_cancel_walk_cb(uv_handle_t* handle, void* arg) {
  auto scheduler = reinterpret_cast<Scheduler*>(arg);
  if (scheduler->running_.load(std::memory_order_relaxed)) {
    scheduler->check_and_cancel(handle);
  }
}

void Scheduler::check_and_cancel(uv_handle_t* handle) {
  if (!handle) {
    return;
  }

  if (handle->type != uv_handle_type::UV_TIMER) {
    return;
  }

  auto timer = reinterpret_cast<Timer*>(handle->data);
  if (!timer) {
    return;
  }

  auto ptr = timer->callback_.lock();
  if (!ptr) {
    return;
  }

  auto callable = ptr->getFunction();

  bool found = false;
  {
    absl::MutexLock lk(&pending_task_list_mtx_);
    for (auto it = pending_task_list_.begin(); it != pending_task_list_.end(); it++) {
      if (it->type_ == TaskType::CANCEL && it->cancelled_callable_ == callable) {
        pending_task_list_.erase(it);
        found = true;
        break;
      }
    }
  }

  if (found) {
    uv_timer_stop(reinterpret_cast<uv_timer_t*>(handle));
    uv_close(handle, &Scheduler::uv_close_callback);
  }
}

void Scheduler::do_daemon() { uv_loop_latch_.countdown(); }

std::string Scheduler::getThreadId() {
  std::stringstream ss;
  ss << std::this_thread::get_id();
  return ss.str();
}

void Scheduler::uv_queue_work_callback(uv_work_t* req) {
  auto timer = static_cast<Timer*>(req->data);
  FunctionalSharePtr callback = timer->callback_.lock();
  if (callback) {
    SPDLOG_TRACE("ThreadID={} executes scheduled task", getThreadId());
    (*(callback->getFunction()))();
  }
  delete req;
}