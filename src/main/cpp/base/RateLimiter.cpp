#include "RateLimiter.h"

ROCKETMQ_NAMESPACE_BEGIN

RateLimiterObserver::RateLimiterObserver() : stopped_(false) {
  tick_thread_ = std::thread([this] {
    while (!stopped_.load(std::memory_order_relaxed)) {
      {
        std::lock_guard<std::mutex> lk(members_mtx_);
        for (auto it = members_.begin(); it != members_.end();) {
          std::shared_ptr<Tick> tick = it->lock();
          if (!tick) {
            it = members_.erase(it);
            continue;
          } else {
            ++it;
          }
          tick->tick();
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  });
}

void RateLimiterObserver::subscribe(const std::shared_ptr<Tick>& tick) {
  std::lock_guard<std::mutex> lk(members_mtx_);
  members_.emplace_back(tick);
}

void RateLimiterObserver::stop() {
  stopped_.store(true, std::memory_order_relaxed);
  if (tick_thread_.joinable()) {
    tick_thread_.join();
  }
}

ROCKETMQ_NAMESPACE_END