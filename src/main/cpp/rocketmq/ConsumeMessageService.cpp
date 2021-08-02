#include "ConsumeMessageService.h"
#include "PushConsumer.h"
#include "LoggerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

ConsumeMessageService::ConsumeMessageService(std::weak_ptr<PushConsumer> consumer, int thread_count,
                                             MessageListener* message_listener)
    : state_(State::CREATED), thread_count_(thread_count),
      pool_(absl::make_unique<grpc::DynamicThreadPool>(thread_count_)), consumer_(std::move(consumer)),
      message_listener_(message_listener) {}

void ConsumeMessageService::start() {
  State expected = State::CREATED;
  if (state_.compare_exchange_strong(expected, State::STARTING, std::memory_order_relaxed)) {
    dispatch_thread_ = std::thread([this] {
      State current_state = state_.load(std::memory_order_relaxed);
      while (State::STOPPED != current_state && State::STOPPING != current_state) {
        dispatch();
        {
          absl::MutexLock lk(&dispatch_mtx_);
          dispatch_cv_.WaitWithTimeout(&dispatch_mtx_, absl::Milliseconds(100));
        }

        // Update current state
        current_state = state_.load(std::memory_order_relaxed);
      }
    });
  }
}

void ConsumeMessageService::signalDispatcher() {
  absl::MutexLock lk(&dispatch_mtx_);
  // Wake up dispatch_thread_
  dispatch_cv_.Signal();
}

void ConsumeMessageService::throttle(const std::string& topic, uint32_t threshold) {
  absl::MutexLock lk(&rate_limiter_table_mtx_);
  std::shared_ptr<RateLimiter<10>> rate_limiter = std::make_shared<RateLimiter<10>>(threshold);
  rate_limiter_table_.insert_or_assign(topic, rate_limiter);
  rate_limiter_observer_.subscribe(rate_limiter);
}

void ConsumeMessageService::shutdown() {
  State expected = State::STOPPING;
  if (state_.compare_exchange_strong(expected, State::STOPPED, std::memory_order_relaxed)) {
    {
      absl::MutexLock lk(&dispatch_mtx_);
      dispatch_cv_.SignalAll();
    }

    if (dispatch_thread_.joinable()) {
      dispatch_thread_.join();
    }

    rate_limiter_observer_.stop();
  }
}

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

void ConsumeMessageService::dispatch() {
  std::shared_ptr<PushConsumer> consumer = consumer_.lock();
  if (!consumer) {
    SPDLOG_WARN("The consumer has already destructed");
    return;
  }

  auto callback = [this](const ProcessQueueSharedPtr& process_queue) { submitConsumeTask(process_queue); };
  consumer->iterateProcessQueue(callback);
}

ROCKETMQ_NAMESPACE_END