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
#ifndef ROCKETMQ_CONSUMER_PULLRESULT_H_
#define ROCKETMQ_CONSUMER_PULLRESULT_H_

#include "MessageExt.h"

namespace rocketmq {

enum PullStatus {
  /**
   * Founded
   */
  FOUND,
  /**
   * No new message can be pull
   */
  NO_NEW_MSG,
  /**
   * Filtering results can not match
   */
  NO_MATCHED_MSG,
  NO_LATEST_MSG,
  /**
   * Illegal offset,may be too big or too small
   */
  OFFSET_ILLEGAL
};

class PullResult {
 public:
  PullResult();
  PullResult(PullStatus status);
  PullResult(PullStatus pullStatus, int64_t nextBeginOffset, int64_t minOffset, int64_t maxOffset);

  PullResult(PullStatus pullStatus,
             int64_t nextBeginOffset,
             int64_t minOffset,
             int64_t maxOffset,
             const std::vector<MessageExtPtr>& msgFoundList);
  PullResult(PullStatus pullStatus,
             int64_t nextBeginOffset,
             int64_t minOffset,
             int64_t maxOffset,
             std::vector<MessageExtPtr>&& msgFoundList);

  virtual ~PullResult();

  std::string toString() const;

  inline PullStatus pull_status() const { return pull_status_; };
  inline void set_pull_status(PullStatus pull_status) { pull_status_ = pull_status; }

  inline int64_t next_begin_offset() const { return next_begin_offset_; };
  inline int64_t min_offset() const { return min_offset_; }
  inline int64_t max_offset() const { return max_offset_; }

  inline std::vector<MessageExtPtr>& msg_found_list() { return msg_found_list_; }
  inline const std::vector<MessageExtPtr>& msg_found_list() const { return msg_found_list_; }

 private:
  PullStatus pull_status_;
  int64_t next_begin_offset_;
  int64_t min_offset_;
  int64_t max_offset_;
  std::vector<MessageExtPtr> msg_found_list_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_PULLRESULT_H_
