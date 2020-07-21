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
#ifndef ROCKETMQ_CONSUMER_PULLRESULTEXT_HPP_
#define ROCKETMQ_CONSUMER_PULLRESULTEXT_HPP_

#include "ByteArray.h"
#include "PullResult.h"

namespace rocketmq {

/**
 * use internal only
 */
class PullResultExt : public PullResult {
 public:
  PullResultExt(PullStatus pullStatus,
                int64_t nextBeginOffset,
                int64_t minOffset,
                int64_t maxOffset,
                int suggestWhichBrokerId)
      : PullResultExt(pullStatus, nextBeginOffset, minOffset, maxOffset, suggestWhichBrokerId, nullptr) {}

  PullResultExt(PullStatus pullStatus,
                int64_t nextBeginOffset,
                int64_t minOffset,
                int64_t maxOffset,
                int suggestWhichBrokerId,
                ByteArrayRef messageBinary)
      : PullResult(pullStatus, nextBeginOffset, minOffset, maxOffset),
        suggert_which_boker_id_(suggestWhichBrokerId),
        message_binary_(messageBinary) {}

  ~PullResultExt() override = default;

 public:
  inline int suggert_which_boker_id() const { return suggert_which_boker_id_; }
  inline ByteArrayRef message_binary() const { return message_binary_; }

 private:
  int suggert_which_boker_id_;
  ByteArrayRef message_binary_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_PULLRESULTEXT_HPP_
