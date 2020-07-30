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
#ifndef ROCKETMQ_PROTOCOL_BODY_LOCKBATCHRESPONSEBODY_HPP_
#define ROCKETMQ_PROTOCOL_BODY_LOCKBATCHRESPONSEBODY_HPP_

#include <algorithm>  // std::move
#include <vector>     // std::vector

#include "Logging.h"
#include "MQMessageQueue.h"
#include "RemotingSerializable.h"

namespace rocketmq {

class LockBatchResponseBody {
 public:
  static LockBatchResponseBody* Decode(const ByteArray& bodyData) {
    Json::Value root = RemotingSerializable::fromJson(bodyData);
    auto& mqs = root["lockOKMQSet"];
    std::unique_ptr<LockBatchResponseBody> body(new LockBatchResponseBody());
    for (const auto& qd : mqs) {
      MQMessageQueue mq(qd["topic"].asString(), qd["brokerName"].asString(), qd["queueId"].asInt());
      LOG_INFO_NEW("LockBatchResponseBody MQ:{}", mq.toString());
      body->lock_ok_mq_set().push_back(std::move(mq));
    }
    return body.release();
  }

 public:
  inline const std::vector<MQMessageQueue>& lock_ok_mq_set() const { return lock_ok_mq_set_; }
  inline std::vector<MQMessageQueue>& lock_ok_mq_set() { return lock_ok_mq_set_; }
  inline void set_lock_ok_mq_set(const std::vector<MQMessageQueue>& lockOKMQSet) { lock_ok_mq_set_ = lockOKMQSet; }
  inline void set_lock_ok_mq_set(std::vector<MQMessageQueue>&& lockOKMQSet) {
    lock_ok_mq_set_ = std::move(lockOKMQSet);
  }

 private:
  std::vector<MQMessageQueue> lock_ok_mq_set_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_BODY_LOCKBATCHRESPONSEBODY_HPP_
