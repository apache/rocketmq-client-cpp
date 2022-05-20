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
#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <system_error>

#include "absl/container/flat_hash_map.h"

#include "ConsumeMessageService.h"
#include "RateLimiter.h"
#include "ThreadPool.h"
#include "rocketmq/State.h"

ROCKETMQ_NAMESPACE_BEGIN

class PushConsumerImpl;

class ConsumeMessageServiceBase : public ConsumeMessageService {
public:
  ConsumeMessageServiceBase(std::weak_ptr<PushConsumerImpl> consumer, int thread_count,
                            MessageListener message_listener);

  ~ConsumeMessageServiceBase() override = default;

  /**
   * Make it noncopyable.
   */
  ConsumeMessageServiceBase(const ConsumeMessageServiceBase& other) = delete;
  ConsumeMessageServiceBase& operator=(const ConsumeMessageServiceBase& other) = delete;

  /**
   * Start the dispatcher thread, which will dispatch messages in process queue to thread pool in form of runnable
   * functor.
   */
  void start() override;

  /**
   * Stop the dispatcher thread and then reset the thread pool.
   */
  void shutdown() override;

  /**
   * Signal dispatcher thread to check new pending messages.
   */
  void signalDispatcher() override;

  /**
   * Set throttle threshold per topic.
   *
   * @param topic
   * @param threshold
   */
  void throttle(const std::string& topic, std::uint32_t threshold) override;

  bool hasConsumeRateLimiter(const std::string& topic) const LOCKS_EXCLUDED(rate_limiter_table_mtx_);

  std::shared_ptr<RateLimiter<10>> rateLimiter(const std::string& topic) const LOCKS_EXCLUDED(rate_limiter_table_mtx_);

protected:
  RateLimiterObserver rate_limiter_observer_;

  mutable absl::flat_hash_map<std::string, std::shared_ptr<RateLimiter<10>>>
      rate_limiter_table_ GUARDED_BY(rate_limiter_table_mtx_);
  mutable absl::Mutex rate_limiter_table_mtx_; // Protects rate_limiter_table_

  std::atomic<State> state_;

  int thread_count_;
  std::unique_ptr<ThreadPool> pool_;
  std::weak_ptr<PushConsumerImpl> consumer_;

  absl::Mutex dispatch_mtx_;
  std::thread dispatch_thread_;
  absl::CondVar dispatch_cv_;

  MessageListener message_listener_;

  /**
   * Dispatch messages to thread pool. Implementation of this function should be sub-class specific.
   */
  void dispatch();
};

ROCKETMQ_NAMESPACE_END