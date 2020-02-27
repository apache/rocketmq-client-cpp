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
#ifndef __PROCESS_QUEUE_H__
#define __PROCESS_QUEUE_H__

#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "MQMessageExt.h"

namespace rocketmq {

class ProcessQueueInfo;

class ProcessQueue;
typedef std::shared_ptr<ProcessQueue> ProcessQueuePtr;

class ROCKETMQCLIENT_API ProcessQueue {
 public:
  static const uint64_t RebalanceLockMaxLiveTime;  // ms
  static const uint64_t RebalanceLockInterval;     // ms

 public:
  ProcessQueue();
  virtual ~ProcessQueue();

  bool isLockExpired() const;
  bool isPullExpired() const;

  void putMessage(std::vector<MQMessageExtPtr2>& msgs);
  int64_t removeMessage(std::vector<MQMessageExtPtr2>& msgs);

  int getCacheMsgCount();
  int64_t getCacheMinOffset();
  int64_t getCacheMaxOffset();

  bool isDropped() const;
  void setDropped(bool dropped);

  bool isLocked() const;
  void setLocked(bool locked);

  int64_t commit();
  void makeMessageToCosumeAgain(std::vector<MQMessageExtPtr2>& msgs);
  void takeMessages(std::vector<MQMessageExtPtr2>& out_msgs, int batchSize);

  void clearAllMsgs();

  uint64_t getLastLockTimestamp() const;
  void setLastLockTimestamp(int64_t lastLockTimestamp);

  std::timed_mutex& getLockConsume();

  uint64_t getLastPullTimestamp() const;
  void setLastPullTimestamp(uint64_t lastPullTimestamp);

  long getTryUnlockTimes();
  void incTryUnlockTimes();

  uint64_t getLastConsumeTimestamp();
  void setLastConsumeTimestamp(uint64_t lastConsumeTimestamp);

  void fillProcessQueueInfo(ProcessQueueInfo& info);

 private:
  static const uint64_t PullMaxIdleTime;

 private:
  std::mutex m_lockTreeMap;
  std::map<int64_t, MQMessageExtPtr2> m_msgTreeMap;
  std::timed_mutex m_lockConsume;
  std::map<int64_t, MQMessageExtPtr2> m_consumingMsgOrderlyTreeMap;
  std::atomic<long> m_tryUnlockTimes;
  volatile int64_t m_queueOffsetMax;
  std::atomic<bool> m_dropped;
  volatile uint64_t m_lastPullTimestamp;
  volatile uint64_t m_lastConsumeTimestamp;
  std::atomic<bool> m_locked;
  volatile uint64_t m_lastLockTimestamp;  // ms
};

}  // namespace rocketmq

#endif  // __PROCESS_QUEUE_H__
