/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "ConsumeMessageServiceBase.h"
#include "LoggerImpl.h"
#include "PushConsumer.h"
#include "ThreadPoolImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

ConsumeMessageServiceBase::ConsumeMessageServiceBase(std::weak_ptr<PushConsumer> consumer, int thread_count,
                                                     MessageListener* message_listener)
    : state_(State::CREATED), thread_count_(thread_count), pool_(absl::make_unique<ThreadPoolImpl>(thread_count_)),
      consumer_(std::move(consumer)), message_listener_(message_listener) {
}

void ConsumeMessageServiceBase::start() {
  State expected = State::CREATED;
  if (state_.compare_exchange_strong(expected, State::STARTING, std::memory_order_relaxed)) {
    pool_->start();
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

void ConsumeMessageServiceBase::signalDispatcher() {
  absl::MutexLock lk(&dispatch_mtx_);
  // Wake up dispatch_thread_
  dispatch_cv_.Signal();
}

void ConsumeMessageServiceBase::throttle(const std::string& topic, std::uint32_t threshold) {
  absl::MutexLock lk(&rate_limiter_table_mtx_);
  std::shared_ptr<RateLimiter<10>> rate_limiter = std::make_shared<RateLimiter<10>>(threshold);
  rate_limiter_table_.insert_or_assign(topic, rate_limiter);
  rate_limiter_observer_.subscribe(rate_limiter);
}

void ConsumeMessageServiceBase::shutdown() {
  State expected = State::STOPPING;
  if (state_.compare_exchange_strong(expected, State::STOPPED, std::memory_order_relaxed)) {
    pool_->shutdown();
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

bool ConsumeMessageServiceBase::hasConsumeRateLimiter(const std::string& topic) const {
  absl::MutexLock lk(&rate_limiter_table_mtx_);
  return rate_limiter_table_.contains(topic);
}

std::shared_ptr<RateLimiter<10>> ConsumeMessageServiceBase::rateLimiter(const std::string& topic) const {
  if (!hasConsumeRateLimiter(topic)) {
    return nullptr;
  }
  absl::MutexLock lk(&rate_limiter_table_mtx_);
  return rate_limiter_table_[topic];
}

void ConsumeMessageServiceBase::dispatch() {
  std::shared_ptr<PushConsumer> consumer = consumer_.lock();
  if (!consumer) {
    SPDLOG_WARN("The consumer has already destructed");
    return;
  }

  auto callback = [this](const ProcessQueueSharedPtr& process_queue) { submitConsumeTask(process_queue); };
  consumer->iterateProcessQueue(callback);
}

ROCKETMQ_NAMESPACE_END