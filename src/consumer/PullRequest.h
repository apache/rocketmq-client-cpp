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
#ifndef ROCKETMQ_CONSUMER_PULLREQUEST_H_
#define ROCKETMQ_CONSUMER_PULLREQUEST_H_

#include <sstream>  // std::stringstream

#include "MQMessageQueue.h"
#include "ProcessQueue.h"

namespace rocketmq {

class PullRequest;
typedef std::shared_ptr<PullRequest> PullRequestPtr;

class ROCKETMQCLIENT_API PullRequest {
 public:
  PullRequest() : next_offset_(0), locked_first_(false) {}
  virtual ~PullRequest() = default;

  inline bool locked_first() const { return locked_first_; }
  inline void set_locked_first(bool lockedFirst) { locked_first_ = lockedFirst; }

  inline const std::string& consumer_group() const { return consumer_group_; }
  inline void set_consumer_group(const std::string& consumerGroup) { consumer_group_ = consumerGroup; }

  inline const MQMessageQueue& message_queue() { return message_queue_; }
  inline void set_message_queue(const MQMessageQueue& messageQueue) { message_queue_ = messageQueue; }

  inline int64_t next_offset() { return next_offset_; }
  inline void set_next_offset(int64_t nextOffset) { next_offset_ = nextOffset; }

  inline ProcessQueuePtr process_queue() { return process_queue_; }
  inline void set_process_queue(ProcessQueuePtr processQueue) { process_queue_ = processQueue; }

  std::string toString() const;

 private:
  std::string consumer_group_;
  MQMessageQueue message_queue_;
  ProcessQueuePtr process_queue_;
  int64_t next_offset_;
  bool locked_first_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_PULLREQUEST_H_
