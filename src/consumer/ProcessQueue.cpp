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
#include "ProcessQueueInfo.h"
#include "UtilAll.h"

namespace rocketmq {

const uint64_t ProcessQueue::RebalanceLockMaxLiveTime = 30000;
const uint64_t ProcessQueue::RebalanceLockInterval = 20000;
const uint64_t ProcessQueue::PullMaxIdleTime = 120000;

ProcessQueue::ProcessQueue()
    : m_queueOffsetMax(0),
      m_dropped(false),
      m_lastPullTimestamp(UtilAll::currentTimeMillis()),
      m_lastConsumeTimestamp(UtilAll::currentTimeMillis()),
      m_locked(false),
      m_lastLockTimestamp(UtilAll::currentTimeMillis()) {}

ProcessQueue::~ProcessQueue() {
  m_msgTreeMap.clear();
  m_consumingMsgOrderlyTreeMap.clear();
}

bool ProcessQueue::isLockExpired() const {
  return (UtilAll::currentTimeMillis() - m_lastLockTimestamp) > RebalanceLockMaxLiveTime;
}

bool ProcessQueue::isPullExpired() const {
  return (UtilAll::currentTimeMillis() - m_lastPullTimestamp) > PullMaxIdleTime;
}

void ProcessQueue::putMessage(const std::vector<MessageExtPtr>& msgs) {
  std::lock_guard<std::mutex> lock(m_lockTreeMap);

  for (const auto& msg : msgs) {
    int64_t offset = msg->getQueueOffset();
    m_msgTreeMap[offset] = msg;
    if (offset > m_queueOffsetMax) {
      m_queueOffsetMax = offset;
    }
  }

  LOG_DEBUG_NEW("ProcessQueue: putMessage m_queueOffsetMax:{}", m_queueOffsetMax);
}

int64_t ProcessQueue::removeMessage(const std::vector<MessageExtPtr>& msgs) {
  int64_t result = -1;
  const auto now = UtilAll::currentTimeMillis();

  std::lock_guard<std::mutex> lock(m_lockTreeMap);
  m_lastConsumeTimestamp = now;

  if (!m_msgTreeMap.empty()) {
    result = m_queueOffsetMax + 1;
    LOG_DEBUG_NEW("offset result is:{}, m_queueOffsetMax is:{}, msgs size:{}", result, m_queueOffsetMax, msgs.size());

    for (auto& msg : msgs) {
      LOG_DEBUG_NEW("remove these msg from m_msgTreeMap, its offset:{}", msg->getQueueOffset());
      m_msgTreeMap.erase(msg->getQueueOffset());
    }

    if (!m_msgTreeMap.empty()) {
      auto it = m_msgTreeMap.begin();
      result = it->first;
    }
  }

  return result;
}

int ProcessQueue::getCacheMsgCount() {
  std::lock_guard<std::mutex> lock(m_lockTreeMap);
  return static_cast<int>(m_msgTreeMap.size());
}

int64_t ProcessQueue::getCacheMinOffset() {
  std::lock_guard<std::mutex> lock(m_lockTreeMap);
  if (m_msgTreeMap.empty()) {
    return 0;
  } else {
    return m_msgTreeMap.begin()->first;
  }
}

int64_t ProcessQueue::getCacheMaxOffset() {
  return m_queueOffsetMax;
}

bool ProcessQueue::isDropped() const {
  return m_dropped.load();
}

void ProcessQueue::setDropped(bool dropped) {
  m_dropped.store(dropped);
}

bool ProcessQueue::isLocked() const {
  return m_locked.load();
}

void ProcessQueue::setLocked(bool locked) {
  m_locked.store(locked);
}

int64_t ProcessQueue::commit() {
  std::lock_guard<std::mutex> lock(m_lockTreeMap);
  if (!m_consumingMsgOrderlyTreeMap.empty()) {
    int64_t offset = (--m_consumingMsgOrderlyTreeMap.end())->first;
    m_consumingMsgOrderlyTreeMap.clear();
    return offset + 1;
  } else {
    return -1;
  }
}

void ProcessQueue::makeMessageToCosumeAgain(std::vector<MessageExtPtr>& msgs) {
  std::lock_guard<std::mutex> lock(m_lockTreeMap);
  for (const auto& msg : msgs) {
    m_msgTreeMap[msg->getQueueOffset()] = msg;
    m_consumingMsgOrderlyTreeMap.erase(msg->getQueueOffset());
  }
}

void ProcessQueue::takeMessages(std::vector<MessageExtPtr>& out_msgs, int batchSize) {
  std::lock_guard<std::mutex> lock(m_lockTreeMap);
  for (int i = 0; i != batchSize; i++) {
    const auto& it = m_msgTreeMap.begin();
    if (it != m_msgTreeMap.end()) {
      out_msgs.push_back(it->second);
      m_consumingMsgOrderlyTreeMap[it->first] = it->second;
      m_msgTreeMap.erase(it);
    }
  }
}

void ProcessQueue::clearAllMsgs() {
  std::lock_guard<std::mutex> lock(m_lockTreeMap);

  if (isDropped()) {
    LOG_DEBUG_NEW("clear m_msgTreeMap as PullRequest had been dropped.");
    m_msgTreeMap.clear();
    m_consumingMsgOrderlyTreeMap.clear();
    m_queueOffsetMax = 0;
  }
}

uint64_t ProcessQueue::getLastLockTimestamp() const {
  return m_lastLockTimestamp;
}

void ProcessQueue::setLastLockTimestamp(int64_t lastLockTimestamp) {
  m_lastLockTimestamp = lastLockTimestamp;
}

std::timed_mutex& ProcessQueue::getLockConsume() {
  return m_lockConsume;
}

uint64_t ProcessQueue::getLastPullTimestamp() const {
  return m_lastPullTimestamp;
}

void ProcessQueue::setLastPullTimestamp(uint64_t lastPullTimestamp) {
  m_lastPullTimestamp = lastPullTimestamp;
}

long ProcessQueue::getTryUnlockTimes() {
  return m_tryUnlockTimes.load();
}

void ProcessQueue::incTryUnlockTimes() {
  m_tryUnlockTimes.fetch_add(1);
}

uint64_t ProcessQueue::getLastConsumeTimestamp() {
  return m_lastConsumeTimestamp;
}

void ProcessQueue::setLastConsumeTimestamp(uint64_t lastConsumeTimestamp) {
  m_lastConsumeTimestamp = lastConsumeTimestamp;
}

void ProcessQueue::fillProcessQueueInfo(ProcessQueueInfo& info) {
  std::lock_guard<std::mutex> lock(m_lockTreeMap);

  if (!m_msgTreeMap.empty()) {
    info.cachedMsgMinOffset = m_msgTreeMap.begin()->first;
    info.cachedMsgMaxOffset = m_queueOffsetMax;
    info.cachedMsgCount = m_msgTreeMap.size();
  }

  if (!m_consumingMsgOrderlyTreeMap.empty()) {
    info.transactionMsgMinOffset = m_consumingMsgOrderlyTreeMap.begin()->first;
    info.transactionMsgMaxOffset = (--m_consumingMsgOrderlyTreeMap.end())->first;
    info.transactionMsgCount = m_consumingMsgOrderlyTreeMap.size();
  }

  info.setLocked(m_locked);
  info.tryUnlockTimes = m_tryUnlockTimes.load();
  info.lastLockTimestamp = m_lastLockTimestamp;

  info.setDroped(m_dropped);
  info.lastPullTimestamp = m_lastPullTimestamp;
  info.lastConsumeTimestamp = m_lastConsumeTimestamp;
}

}  // namespace rocketmq
