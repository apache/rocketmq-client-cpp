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
#ifndef __MESSAGE_QUEUE_LOCK__
#define __MESSAGE_QUEUE_LOCK__

#include <map>
#include <memory>
#include <mutex>

#include "MQMessageQueue.h"

namespace rocketmq {

/**
 * Message lock, strictly ensure the single queue only one thread at a time consuming
 */
class MessageQueueLock {
 public:
  std::shared_ptr<std::mutex> fetchLockObject(const MQMessageQueue& mq) {
    std::lock_guard<std::mutex> lock(m_mqLockTableMutex);
    if (m_mqLockTable.find(mq) == m_mqLockTable.end()) {
      m_mqLockTable.emplace(mq, std::shared_ptr<std::mutex>(new std::mutex()));
    }
    return m_mqLockTable[mq];
  }

 private:
  std::map<MQMessageQueue, std::shared_ptr<std::mutex>> m_mqLockTable;
  std::mutex m_mqLockTableMutex;
};

}  // namespace rocketmq

#endif  // __MESSAGE_QUEUE_LOCK__
