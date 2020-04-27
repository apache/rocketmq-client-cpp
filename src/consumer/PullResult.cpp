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

#include "UtilAll.h"

namespace rocketmq {

static const char* EnumStrings[] = {"FOUND", "NO_NEW_MSG", "NO_MATCHED_MSG",
                                    "NO_LATEST_MSG"
                                    "OFFSET_ILLEGAL",
                                    "BROKER_TIMEOUT"};

PullResult::PullResult() : pullStatus(NO_MATCHED_MSG), nextBeginOffset(0), minOffset(0), maxOffset(0) {}

PullResult::PullResult(PullStatus status) : pullStatus(status), nextBeginOffset(0), minOffset(0), maxOffset(0) {}

PullResult::PullResult(PullStatus pullStatus, int64_t nextBeginOffset, int64_t minOffset, int64_t maxOffset)
    : pullStatus(pullStatus), nextBeginOffset(nextBeginOffset), minOffset(minOffset), maxOffset(maxOffset) {}

PullResult::PullResult(PullStatus pullStatus,
                       int64_t nextBeginOffset,
                       int64_t minOffset,
                       int64_t maxOffset,
                       const std::vector<MQMessageExtPtr2>& src)
    : pullStatus(pullStatus),
      nextBeginOffset(nextBeginOffset),
      minOffset(minOffset),
      maxOffset(maxOffset),
      msgFoundList(src) {}

PullResult::PullResult(PullStatus pullStatus,
                       int64_t nextBeginOffset,
                       int64_t minOffset,
                       int64_t maxOffset,
                       std::vector<MQMessageExtPtr2>&& src)
    : pullStatus(pullStatus),
      nextBeginOffset(nextBeginOffset),
      minOffset(minOffset),
      maxOffset(maxOffset),
      msgFoundList(std::forward<std::vector<MQMessageExtPtr2>>(src)) {}

PullResult::~PullResult() = default;

std::string PullResult::toString() const {
  std::stringstream ss;
  ss << "PullResult [ pullStatus=" << EnumStrings[pullStatus] << ", nextBeginOffset=" << nextBeginOffset
     << ", minOffset=" << minOffset << ", maxOffset=" << maxOffset << ", msgFoundList=" << msgFoundList.size() << " ]";
  return ss.str();
}

}  // namespace rocketmq
