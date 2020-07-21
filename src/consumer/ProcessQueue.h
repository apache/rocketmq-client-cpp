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
#ifndef ROCKETMQ_CONSUMER_PROCESSQUEUE_H_
#define ROCKETMQ_CONSUMER_PROCESSQUEUE_H_

#include <atomic>  // std::atomic
#include <map>     // std::map
#include <memory>  // std::shared_ptr
#include <mutex>   // std::mutex
#include <vector>  // std::vector

#include "MessageExt.h"

namespace rocketmq {

class ProcessQueueInfo;

class ProcessQueue;
typedef std::shared_ptr<ProcessQueue> ProcessQueuePtr;

class ROCKETMQCLIENT_API ProcessQueue {
 public:
  static const uint64_t REBALANCE_LOCK_MAX_LIVE_TIME;  // ms
  static const uint64_t REBALANCE_LOCK_INTERVAL;       // ms

 public:
  ProcessQueue();
  virtual ~ProcessQueue();

  bool isLockExpired() const;
  bool isPullExpired() const;

  void putMessage(const std::vector<MessageExtPtr>& msgs);
  int64_t removeMessage(const std::vector<MessageExtPtr>& msgs);

  int getCacheMsgCount();
  int64_t getCacheMinOffset();
  int64_t getCacheMaxOffset();

  int64_t commit();
  void makeMessageToCosumeAgain(std::vector<MessageExtPtr>& msgs);
  void takeMessages(std::vector<MessageExtPtr>& out_msgs, int batchSize);

  void clearAllMsgs();

  void fillProcessQueueInfo(ProcessQueueInfo& info);

 public:
  inline std::timed_mutex& lock_consume() { return lock_consume_; }

  inline long try_unlock_times() const { return try_unlock_times_.load(); }
  inline void inc_try_unlock_times() { try_unlock_times_.fetch_add(1); }

  inline bool dropped() const { return dropped_.load(); }
  inline void set_dropped(bool dropped) { dropped_.store(dropped); }

  inline uint64_t last_pull_timestamp() const { return last_pull_timestamp_; }
  inline void set_last_pull_timestamp(uint64_t lastPullTimestamp) { last_pull_timestamp_ = lastPullTimestamp; }

  inline uint64_t last_consume_timestamp() const { return last_consume_timestamp_; }
  inline void set_last_consume_timestamp(uint64_t lastConsumeTimestamp) {
    last_consume_timestamp_ = lastConsumeTimestamp;
  }

  inline bool locked() const { return locked_.load(); }
  inline void set_locked(bool locked) { locked_.store(locked); }

  inline uint64_t last_lock_timestamp() const { return last_lock_timestamp_; }
  inline void set_last_lock_timestamp(int64_t lastLockTimestamp) { last_lock_timestamp_ = lastLockTimestamp; }

 private:
  std::mutex lock_tree_map_;
  std::map<int64_t, MessageExtPtr> msg_tree_map_;
  std::timed_mutex lock_consume_;
  std::map<int64_t, MessageExtPtr> consuming_msg_orderly_tree_map_;
  std::atomic<long> try_unlock_times_;
  volatile int64_t queue_offset_max_;
  std::atomic<bool> dropped_;
  volatile uint64_t last_pull_timestamp_;
  volatile uint64_t last_consume_timestamp_;
  std::atomic<bool> locked_;
  volatile uint64_t last_lock_timestamp_;  // ms
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_PROCESSQUEUE_H_
