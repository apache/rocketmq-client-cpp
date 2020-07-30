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
#ifndef ROCKETMQ_PROTOCOL_BODY_RESETOFFSETBODY_HPP_
#define ROCKETMQ_PROTOCOL_BODY_RESETOFFSETBODY_HPP_

#include <map>  // std::map

#include "MQMessageQueue.h"
#include "RemotingSerializable.h"

namespace rocketmq {

class ResetOffsetBody {
 public:
  static ResetOffsetBody* Decode(const ByteArray& bodyData) {
    // FIXME: object as key
    Json::Value root = RemotingSerializable::fromJson(bodyData);
    auto& qds = root["offsetTable"];
    std::unique_ptr<ResetOffsetBody> body(new ResetOffsetBody());
    Json::Value::Members members = qds.getMemberNames();
    for (const auto& member : members) {
      Json::Value key = RemotingSerializable::fromJson(member);
      MQMessageQueue mq(key["topic"].asString(), key["brokerName"].asString(), key["queueId"].asInt());
      int64_t offset = qds[member].asInt64();
      body->offset_table_.emplace(std::move(mq), offset);
    }
    return body.release();
  }

 public:
  std::map<MQMessageQueue, int64_t>& offset_table() { return offset_table_; }

 private:
  std::map<MQMessageQueue, int64_t> offset_table_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_BODY_RESETOFFSETBODY_HPP_
