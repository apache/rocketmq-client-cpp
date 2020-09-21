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
#ifndef ROCKETMQ_CONSUMER_ASSIGNEDMESSAGEQUEUE_H_
#define ROCKETMQ_CONSUMER_ASSIGNEDMESSAGEQUEUE_H_

#include <algorithm>  // std::move, std::binary_search
#include <mutex>      // std::mutex

#include "MQMessageQueue.h"
#include "ProcessQueue.h"
#include "RebalanceImpl.h"

namespace rocketmq {

class MessageQueueState {
 public:
  MessageQueueState(const MQMessageQueue& message_queue, ProcessQueuePtr process_queue)
      : message_queue_(message_queue),
        process_queue_(std::move(process_queue)),
        paused_(false),
        pull_offset_(-1),
        consume_offset_(-1),
        seek_offset_(-1) {}

  inline const MQMessageQueue& message_queue() const { return message_queue_; }
  inline void set_message_queue(const MQMessageQueue message_queue) { message_queue_ = message_queue; }

  inline ProcessQueuePtr process_queue() const { return process_queue_; }
  inline void process_queue(ProcessQueuePtr process_queue) { process_queue_ = std::move(process_queue); }

  inline bool is_paused() const { return paused_; }
  inline void set_paused(bool paused) { paused_ = paused; }

  inline int64_t pull_offset() const { return pull_offset_; }
  inline void set_pull_offset(int64_t pull_offset) { pull_offset_ = pull_offset; }

  inline int64_t consume_offset() const { return consume_offset_; }
  inline void set_consume_offset(int64_t consume_offset) { consume_offset_ = consume_offset; }

  inline int64_t seek_offset() const { return seek_offset_; }
  inline void set_seek_offset(int64_t seek_offset) { seek_offset_ = seek_offset; }

 private:
  MQMessageQueue message_queue_;
  ProcessQueuePtr process_queue_;
  volatile bool paused_;
  volatile int64_t pull_offset_;
  volatile int64_t consume_offset_;
  volatile int64_t seek_offset_;
};

class AssignedMessageQueue {
 public:
  std::vector<MQMessageQueue> messageQueues() {
    std::vector<MQMessageQueue> mqs;
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    for (const auto& it : assigned_message_queue_state_) {
      mqs.push_back(it.first);
    }
    return mqs;
  }

  bool isPaused(const MQMessageQueue& message_queue) {
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    auto it = assigned_message_queue_state_.find(message_queue);
    if (it != assigned_message_queue_state_.end()) {
      auto& message_queue_state = it->second;
      return message_queue_state.is_paused();
    }
    return true;
  }

  void pause(const std::vector<MQMessageQueue>& message_queues) {
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    for (const auto& message_queue : message_queues) {
      auto it = assigned_message_queue_state_.find(message_queue);
      if (it != assigned_message_queue_state_.end()) {
        auto& message_queue_state = it->second;
        message_queue_state.set_paused(true);
      }
    }
  }

  void resume(const std::vector<MQMessageQueue>& message_queues) {
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    for (const auto& message_queue : message_queues) {
      auto it = assigned_message_queue_state_.find(message_queue);
      if (it != assigned_message_queue_state_.end()) {
        auto& message_queue_state = it->second;
        message_queue_state.set_paused(false);
      }
    }
  }

  ProcessQueuePtr getProcessQueue(const MQMessageQueue& message_queue) {
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    auto it = assigned_message_queue_state_.find(message_queue);
    if (it != assigned_message_queue_state_.end()) {
      auto& message_queue_state = it->second;
      return message_queue_state.process_queue();
    }
    return nullptr;
  }

  int64_t getPullOffset(const MQMessageQueue& message_queue) {
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    auto it = assigned_message_queue_state_.find(message_queue);
    if (it != assigned_message_queue_state_.end()) {
      auto& message_queue_state = it->second;
      return message_queue_state.pull_offset();
    }
    return -1;
  }

  void updatePullOffset(const MQMessageQueue& message_queue, int64_t offset) {
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    auto it = assigned_message_queue_state_.find(message_queue);
    if (it != assigned_message_queue_state_.end()) {
      auto& message_queue_state = it->second;
      return message_queue_state.set_pull_offset(offset);
    }
  }

  int64_t getConsumerOffset(const MQMessageQueue& message_queue) {
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    auto it = assigned_message_queue_state_.find(message_queue);
    if (it != assigned_message_queue_state_.end()) {
      auto& message_queue_state = it->second;
      return message_queue_state.consume_offset();
    }
    return -1;
  }

  void updateConsumeOffset(const MQMessageQueue& message_queue, int64_t offset) {
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    auto it = assigned_message_queue_state_.find(message_queue);
    if (it != assigned_message_queue_state_.end()) {
      auto& message_queue_state = it->second;
      return message_queue_state.set_consume_offset(offset);
    }
  }

  int64_t getSeekOffset(const MQMessageQueue& message_queue) {
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    auto it = assigned_message_queue_state_.find(message_queue);
    if (it != assigned_message_queue_state_.end()) {
      auto& message_queue_state = it->second;
      return message_queue_state.seek_offset();
    }
    return -1;
  }

  void setSeekOffset(const MQMessageQueue& message_queue, int64_t offset) {
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    auto it = assigned_message_queue_state_.find(message_queue);
    if (it != assigned_message_queue_state_.end()) {
      auto& message_queue_state = it->second;
      return message_queue_state.set_seek_offset(offset);
    }
  }

  void updateAssignedMessageQueue(const std::string& topic, std::vector<MQMessageQueue>& assigned) {
    std::sort(assigned.begin(), assigned.end());
    std::lock_guard<std::mutex> lock(assigned_message_queue_state_mutex_);
    for (auto it = assigned_message_queue_state_.begin(); it != assigned_message_queue_state_.end();) {
      auto& mq = it->first;
      if (mq.topic() == topic) {
        if (!std::binary_search(assigned.begin(), assigned.end(), mq)) {
          it = assigned_message_queue_state_.erase(it);
          continue;
        }
      }
      it++;
    }
    addAssignedMessageQueue(assigned);
  }

 private:
  void addAssignedMessageQueue(const std::vector<MQMessageQueue>& assigned) {
    for (const auto& message_queue : assigned) {
      if (assigned_message_queue_state_.find(message_queue) == assigned_message_queue_state_.end()) {
        ProcessQueuePtr process_queue;
        if (rebalance_impl_ != nullptr) {
          process_queue = rebalance_impl_->getProcessQueue(message_queue);
        }
        if (nullptr == process_queue) {
          process_queue.reset(new ProcessQueue());
        }
        assigned_message_queue_state_.emplace(message_queue, MessageQueueState(message_queue, process_queue));
      }
    }
  }

 public:
  inline void set_rebalance_impl(RebalanceImpl* rebalance_impl) { rebalance_impl_ = rebalance_impl; }

 private:
  std::map<MQMessageQueue, MessageQueueState> assigned_message_queue_state_;
  std::mutex assigned_message_queue_state_mutex_;
  RebalanceImpl* rebalance_impl_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_ASSIGNEDMESSAGEQUEUE_H_
