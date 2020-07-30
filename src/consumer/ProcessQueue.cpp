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
#include "ProcessQueue.h"

#include "Logging.h"
#include "protocol/body/ProcessQueueInfo.hpp"
#include "UtilAll.h"

static const uint64_t PULL_MAX_IDLE_TIME = 120000;  // ms

namespace rocketmq {

const uint64_t ProcessQueue::REBALANCE_LOCK_MAX_LIVE_TIME = 30000;
const uint64_t ProcessQueue::REBALANCE_LOCK_INTERVAL = 20000;

ProcessQueue::ProcessQueue()
    : queue_offset_max_(0),
      dropped_(false),
      last_pull_timestamp_(UtilAll::currentTimeMillis()),
      last_consume_timestamp_(UtilAll::currentTimeMillis()),
      locked_(false),
      last_lock_timestamp_(UtilAll::currentTimeMillis()) {}

ProcessQueue::~ProcessQueue() {
  msg_tree_map_.clear();
  consuming_msg_orderly_tree_map_.clear();
}

bool ProcessQueue::isLockExpired() const {
  return (UtilAll::currentTimeMillis() - last_lock_timestamp_) > REBALANCE_LOCK_MAX_LIVE_TIME;
}

bool ProcessQueue::isPullExpired() const {
  return (UtilAll::currentTimeMillis() - last_pull_timestamp_) > PULL_MAX_IDLE_TIME;
}

void ProcessQueue::putMessage(const std::vector<MessageExtPtr>& msgs) {
  std::lock_guard<std::mutex> lock(lock_tree_map_);

  for (const auto& msg : msgs) {
    int64_t offset = msg->queue_offset();
    msg_tree_map_[offset] = msg;
    if (offset > queue_offset_max_) {
      queue_offset_max_ = offset;
    }
  }

  LOG_DEBUG_NEW("ProcessQueue: putMessage queue_offset_max:{}", queue_offset_max_);
}

int64_t ProcessQueue::removeMessage(const std::vector<MessageExtPtr>& msgs) {
  int64_t result = -1;
  const auto now = UtilAll::currentTimeMillis();

  std::lock_guard<std::mutex> lock(lock_tree_map_);
  last_consume_timestamp_ = now;

  if (!msg_tree_map_.empty()) {
    result = queue_offset_max_ + 1;
    LOG_DEBUG_NEW("offset result is:{}, queue_offset_max is:{}, msgs size:{}", result, queue_offset_max_, msgs.size());

    for (auto& msg : msgs) {
      LOG_DEBUG_NEW("remove these msg from msg_tree_map, its offset:{}", msg->queue_offset());
      msg_tree_map_.erase(msg->queue_offset());
    }

    if (!msg_tree_map_.empty()) {
      auto it = msg_tree_map_.begin();
      result = it->first;
    }
  }

  return result;
}

int ProcessQueue::getCacheMsgCount() {
  std::lock_guard<std::mutex> lock(lock_tree_map_);
  return static_cast<int>(msg_tree_map_.size() + consuming_msg_orderly_tree_map_.size());
}

int64_t ProcessQueue::getCacheMinOffset() {
  std::lock_guard<std::mutex> lock(lock_tree_map_);
  if (msg_tree_map_.empty() && consuming_msg_orderly_tree_map_.empty()) {
    return 0;
  } else if (!consuming_msg_orderly_tree_map_.empty()) {
    return consuming_msg_orderly_tree_map_.begin()->first;
  } else {
    return msg_tree_map_.begin()->first;
  }
}

int64_t ProcessQueue::getCacheMaxOffset() {
  return queue_offset_max_;
}

int64_t ProcessQueue::commit() {
  std::lock_guard<std::mutex> lock(lock_tree_map_);
  if (!consuming_msg_orderly_tree_map_.empty()) {
    int64_t offset = (--consuming_msg_orderly_tree_map_.end())->first;
    consuming_msg_orderly_tree_map_.clear();
    return offset + 1;
  } else {
    return -1;
  }
}

void ProcessQueue::makeMessageToCosumeAgain(std::vector<MessageExtPtr>& msgs) {
  std::lock_guard<std::mutex> lock(lock_tree_map_);
  for (const auto& msg : msgs) {
    msg_tree_map_[msg->queue_offset()] = msg;
    consuming_msg_orderly_tree_map_.erase(msg->queue_offset());
  }
}

void ProcessQueue::takeMessages(std::vector<MessageExtPtr>& out_msgs, int batchSize) {
  std::lock_guard<std::mutex> lock(lock_tree_map_);
  for (auto it = msg_tree_map_.begin(); it != msg_tree_map_.end() && batchSize--;) {
    out_msgs.push_back(it->second);
    consuming_msg_orderly_tree_map_[it->first] = it->second;
    it = msg_tree_map_.erase(it);
  }
}

void ProcessQueue::clearAllMsgs() {
  std::lock_guard<std::mutex> lock(lock_tree_map_);

  if (dropped()) {
    LOG_DEBUG_NEW("clear msg_tree_map as PullRequest had been dropped.");
    msg_tree_map_.clear();
    consuming_msg_orderly_tree_map_.clear();
    queue_offset_max_ = 0;
  }
}

void ProcessQueue::fillProcessQueueInfo(ProcessQueueInfo& info) {
  std::lock_guard<std::mutex> lock(lock_tree_map_);

  if (!msg_tree_map_.empty()) {
    info.cachedMsgMinOffset = msg_tree_map_.begin()->first;
    info.cachedMsgMaxOffset = queue_offset_max_;
    info.cachedMsgCount = msg_tree_map_.size();
  }

  if (!consuming_msg_orderly_tree_map_.empty()) {
    info.transactionMsgMinOffset = consuming_msg_orderly_tree_map_.begin()->first;
    info.transactionMsgMaxOffset = (--consuming_msg_orderly_tree_map_.end())->first;
    info.transactionMsgCount = consuming_msg_orderly_tree_map_.size();
  }

  info.setLocked(locked_);
  info.tryUnlockTimes = try_unlock_times_.load();
  info.lastLockTimestamp = last_lock_timestamp_;

  info.setDroped(dropped_);
  info.lastPullTimestamp = last_pull_timestamp_;
  info.lastConsumeTimestamp = last_consume_timestamp_;
}

}  // namespace rocketmq
