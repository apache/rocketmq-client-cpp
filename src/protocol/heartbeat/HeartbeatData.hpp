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
#ifndef ROCKETMQ_PROTOCOL_HEARTBEAT_HEARTBEATDATA_H_
#define ROCKETMQ_PROTOCOL_HEARTBEAT_HEARTBEATDATA_H_

#include <string>  // std::string
#include <vector>  // std::vector

#include "RemotingSerializable.h"
#include "ConsumerData.hpp"
#include "ProducerData.hpp"

namespace rocketmq {

class HeartbeatData : public RemotingSerializable {
 public:
  std::string encode() {
    Json::Value root;

    // id
    root["clientID"] = client_id_;

    // consumer
    for (const auto& cd : consumer_data_set_) {
      root["consumerDataSet"].append(cd.toJson());
    }

    // producer
    for (const auto& pd : producer_data_set_) {
      root["producerDataSet"].append(pd.toJson());
    }

    // output
    return RemotingSerializable::toJson(root);
  }

 public:
  inline void set_client_id(const std::string& clientID) { client_id_ = clientID; }

  inline std::vector<ConsumerData>& consumer_data_set() { return consumer_data_set_; }

  inline std::vector<ProducerData>& producer_data_set() { return producer_data_set_; }

 private:
  std::string client_id_;
  std::vector<ConsumerData> consumer_data_set_;
  std::vector<ProducerData> producer_data_set_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_HEARTBEAT_HEARTBEATDATA_H_
