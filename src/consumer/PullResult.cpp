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
#include "PullResult.h"

#include <algorithm>  // std::move
#include <sstream>    // std::stringstream

#include "UtilAll.h"

static const char* kPullStatusStrings[] = {
    "FOUND", "NO_NEW_MSG", "NO_MATCHED_MSG", "NO_LATEST_MSG", "OFFSET_ILLEGAL",
};

namespace rocketmq {

PullResult::PullResult() : pull_status_(NO_MATCHED_MSG), next_begin_offset_(0), min_offset_(0), max_offset_(0) {}

PullResult::PullResult(PullStatus status)
    : pull_status_(status), next_begin_offset_(0), min_offset_(0), max_offset_(0) {}

PullResult::PullResult(PullStatus pullStatus, int64_t nextBeginOffset, int64_t minOffset, int64_t maxOffset)
    : pull_status_(pullStatus), next_begin_offset_(nextBeginOffset), min_offset_(minOffset), max_offset_(maxOffset) {}

PullResult::PullResult(PullStatus pullStatus,
                       int64_t nextBeginOffset,
                       int64_t minOffset,
                       int64_t maxOffset,
                       const std::vector<MessageExtPtr>& msgFoundList)
    : pull_status_(pullStatus),
      next_begin_offset_(nextBeginOffset),
      min_offset_(minOffset),
      max_offset_(maxOffset),
      msg_found_list_(msgFoundList) {}

PullResult::PullResult(PullStatus pullStatus,
                       int64_t nextBeginOffset,
                       int64_t minOffset,
                       int64_t maxOffset,
                       std::vector<MessageExtPtr>&& msgFoundList)
    : pull_status_(pullStatus),
      next_begin_offset_(nextBeginOffset),
      min_offset_(minOffset),
      max_offset_(maxOffset),
      msg_found_list_(std::move(msgFoundList)) {}

PullResult::~PullResult() = default;

std::string PullResult::toString() const {
  std::stringstream ss;
  ss << "PullResult [ pullStatus=" << kPullStatusStrings[pull_status_] << ", nextBeginOffset=" << next_begin_offset_
     << ", minOffset=" << min_offset_ << ", maxOffset=" << max_offset_ << ", msgFoundList=" << msg_found_list_.size()
     << " ]";
  return ss.str();
}

}  // namespace rocketmq
