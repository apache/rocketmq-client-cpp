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
#ifndef __PULL_REQUEST_H__
#define __PULL_REQUEST_H__

#include <atomic>
#include <memory>
#include <mutex>

#include "MQMessageQueue.h"
#include "ProcessQueue.h"

namespace rocketmq {

class PullRequest;
typedef std::shared_ptr<PullRequest> PullRequestPtr;

class ROCKETMQCLIENT_API PullRequest {
 public:
  PullRequest();
  virtual ~PullRequest();

  bool isLockedFirst() const;
  void setLockedFirst(bool lockedFirst);

  const std::string& getConsumerGroup() const;
  void setConsumerGroup(const std::string& consumerGroup);

  const MQMessageQueue& getMessageQueue();
  void setMessageQueue(const MQMessageQueue& messageQueue);

  int64_t getNextOffset();
  void setNextOffset(int64_t nextOffset);

  ProcessQueuePtr getProcessQueue();
  void setProcessQueue(ProcessQueuePtr processQueue);

  std::string toString() const {
    std::stringstream ss;
    ss << "PullRequest [consumerGroup=" << m_consumerGroup << ", messageQueue=" << m_messageQueue.toString()
       << ", nextOffset=" << m_nextOffset << "]";
    return ss.str();
  }

 private:
  std::string m_consumerGroup;
  MQMessageQueue m_messageQueue;
  ProcessQueuePtr m_processQueue;
  int64_t m_nextOffset;
  bool m_lockedFirst;
};

}  // namespace rocketmq

#endif  // __PULL_REQUEST_H__
