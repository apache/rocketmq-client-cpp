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
#ifndef ROCKETMQ_CONSUMER_MESSAGEQUEUELOCK_HPP_
#define ROCKETMQ_CONSUMER_MESSAGEQUEUELOCK_HPP_

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
    std::lock_guard<std::mutex> lock(mq_lock_table_mutex_);
    const auto& it = mq_lock_table_.find(mq);
    if (it != mq_lock_table_.end()) {
      return it->second;
    } else {
      return mq_lock_table_[mq] = std::make_shared<std::mutex>();
    }
  }

 private:
  std::map<MQMessageQueue, std::shared_ptr<std::mutex>> mq_lock_table_;
  std::mutex mq_lock_table_mutex_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_MESSAGEQUEUELOCK_HPP_
