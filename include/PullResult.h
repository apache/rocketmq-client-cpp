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
#ifndef __PULL_RESULT_H__
#define __PULL_RESULT_H__

#include <memory>
#include <sstream>

#include "MQMessageExt.h"

namespace rocketmq {

enum PullStatus {
  FOUND,
  NO_NEW_MSG,
  NO_MATCHED_MSG,
  NO_LATEST_MSG,
  OFFSET_ILLEGAL,
  BROKER_TIMEOUT  // indicate pull request timeout or received NULL response
};

class ROCKETMQCLIENT_API PullResult {
 public:
  PullResult();
  PullResult(PullStatus status);
  PullResult(PullStatus pullStatus, int64_t nextBeginOffset, int64_t minOffset, int64_t maxOffset);

  PullResult(PullStatus pullStatus,
             int64_t nextBeginOffset,
             int64_t minOffset,
             int64_t maxOffset,
             const std::vector<MQMessageExtPtr2>& src);
  PullResult(PullStatus pullStatus,
             int64_t nextBeginOffset,
             int64_t minOffset,
             int64_t maxOffset,
             std::vector<MQMessageExtPtr2>&& src);

  virtual ~PullResult();

  std::string toString() const;

 public:
  PullStatus pullStatus;
  int64_t nextBeginOffset;
  int64_t minOffset;
  int64_t maxOffset;
  std::vector<MQMessageExtPtr2> msgFoundList;
};

}  // namespace rocketmq

#endif  // __PULL_RESULT_H__
