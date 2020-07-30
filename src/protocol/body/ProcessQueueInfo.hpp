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
#ifndef ROCKETMQ_PROTOCOL_BODY_PROCESS_QUEUE_INFO_HPP_
#define ROCKETMQ_PROTOCOL_BODY_PROCESS_QUEUE_INFO_HPP_

#include <json/json.h>

#include "UtilAll.h"

namespace rocketmq {

class ProcessQueueInfo {
 public:
  ProcessQueueInfo()
      : commitOffset(0),
        cachedMsgMinOffset(0),
        cachedMsgMaxOffset(0),
        cachedMsgCount(0),
        transactionMsgMinOffset(0),
        transactionMsgMaxOffset(0),
        transactionMsgCount(0),
        locked(false),
        tryUnlockTimes(0),
        lastLockTimestamp(0),
        droped(false),
        lastPullTimestamp(0),
        lastConsumeTimestamp(0) {}

  virtual ~ProcessQueueInfo() = default;

 public:
  const uint64_t getCommitOffset() const { return commitOffset; }

  void setCommitOffset(uint64_t commitOffset) { this->commitOffset = commitOffset; }

  void setLocked(bool locked) { this->locked = locked; }

  const bool isLocked() const { return locked; }

  void setDroped(bool droped) { this->droped = droped; }

  const bool isDroped() const { return droped; }

  Json::Value toJson() const {
    Json::Value outJson;
    outJson["commitOffset"] = UtilAll::to_string(commitOffset);
    outJson["cachedMsgMinOffset"] = UtilAll::to_string(cachedMsgMinOffset);
    outJson["cachedMsgMaxOffset"] = UtilAll::to_string(cachedMsgMaxOffset);
    outJson["cachedMsgCount"] = cachedMsgCount;
    outJson["transactionMsgMinOffset"] = UtilAll::to_string(transactionMsgMinOffset);
    outJson["transactionMsgMaxOffset"] = UtilAll::to_string(transactionMsgMaxOffset);
    outJson["transactionMsgCount"] = transactionMsgCount;
    outJson["locked"] = locked;
    outJson["tryUnlockTimes"] = tryUnlockTimes;
    outJson["lastLockTimestamp"] = UtilAll::to_string(lastLockTimestamp);
    outJson["droped"] = droped;
    outJson["lastPullTimestamp"] = UtilAll::to_string(lastPullTimestamp);
    outJson["lastConsumeTimestamp"] = UtilAll::to_string(lastConsumeTimestamp);
    return outJson;
  }

 public:
  uint64_t commitOffset;
  uint64_t cachedMsgMinOffset;
  uint64_t cachedMsgMaxOffset;
  int32_t cachedMsgCount;
  uint64_t transactionMsgMinOffset;
  uint64_t transactionMsgMaxOffset;
  int32_t transactionMsgCount;
  bool locked;
  int32_t tryUnlockTimes;
  uint64_t lastLockTimestamp;

  bool droped;
  uint64_t lastPullTimestamp;
  uint64_t lastConsumeTimestamp;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_BODY_PROCESS_QUEUE_INFO_HPP_
