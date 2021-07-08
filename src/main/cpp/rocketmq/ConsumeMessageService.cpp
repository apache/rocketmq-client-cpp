#include "ConsumeMessageService.h"

ROCKETMQ_NAMESPACE_BEGIN

void ConsumeMessageService::throttle(const std::string& topic, uint32_t threshold) {
  absl::MutexLock lk(&rate_limiter_table_mtx_);
  std::shared_ptr<RateLimiter<10>> rate_limiter = std::make_shared<RateLimiter<10>>(threshold);
  rate_limiter_table_.insert_or_assign(topic, rate_limiter);
  rate_limiter_observer_.subscribe(rate_limiter);
}

void ConsumeMessageService::shutdown() { rate_limiter_observer_.stop(); }

bool ConsumeMessageService::hasConsumeRateLimiter(const std::string& topic) const {
  absl::MutexLock lk(&rate_limiter_table_mtx_);
  return rate_limiter_table_.contains(topic);
}

std::shared_ptr<RateLimiter<10>> ConsumeMessageService::rateLimiter(const std::string& topic) const {
  if (!hasConsumeRateLimiter(topic)) {
    return nullptr;
  }
  absl::MutexLock lk(&rate_limiter_table_mtx_);
  return rate_limiter_table_[topic];
}

ROCKETMQ_NAMESPACE_END