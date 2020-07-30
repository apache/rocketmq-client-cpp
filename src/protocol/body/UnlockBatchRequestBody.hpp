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
#ifndef ROCKETMQ_PROTOCOL_BODY_UNLOCKBATCHREQUESTBODY_HPP_
#define ROCKETMQ_PROTOCOL_BODY_UNLOCKBATCHREQUESTBODY_HPP_

#include <algorithm>  // std::move
#include <vector>     // std::vector

#include "MessageQueue.hpp"
#include "RemotingSerializable.h"

namespace rocketmq {

class UnlockBatchRequestBody : public RemotingSerializable {
 public:
  std::string encode() override {
    Json::Value root;
    root["consumerGroup"] = consumer_group_;
    root["clientId"] = client_id_;

    for (const auto& mq : mq_set_) {
      root["mqSet"].append(rocketmq::toJson(mq));
    }

    return RemotingSerializable::toJson(root);
  }

 public:
  inline const std::string& consumer_group() { return consumer_group_; }
  inline void set_consumer_group(const std::string& consumerGroup) { consumer_group_ = consumerGroup; }

  inline const std::string& client_id() { return client_id_; }
  inline void set_client_id(const std::string& clientId) { client_id_ = clientId; }

  inline std::vector<MQMessageQueue>& mq_set() { return mq_set_; }
  inline void set_mq_set(const std::vector<MQMessageQueue>& mq_set) { mq_set_ = mq_set; }
  inline void set_mq_set(std::vector<MQMessageQueue>&& mq_set) { mq_set_ = std::move(mq_set); }

 private:
  std::string consumer_group_;
  std::string client_id_;
  std::vector<MQMessageQueue> mq_set_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_BODY_UNLOCKBATCHREQUESTBODY_HPP_
