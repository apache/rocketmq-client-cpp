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
#ifndef ROCKETMQ_PROTOCOL_HEARTBEAT_PRODUCERDATA_H_
#define ROCKETMQ_PROTOCOL_HEARTBEAT_PRODUCERDATA_H_

#include <string>  // std::string

#include <json/json.h>

namespace rocketmq {

class ProducerData {
 public:
  ProducerData(const std::string& group_name) : group_name_(group_name) {}

  bool operator<(const ProducerData& other) const { return group_name_ < other.group_name_; }

  Json::Value toJson() const {
    Json::Value root;
    root["groupName"] = group_name_;
    return root;
  }

 public:
  inline const std::string& group_name() const { return group_name_; }
  inline void group_name(const std::string& group_name) { group_name_ = group_name; }

 private:
  std::string group_name_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_HEARTBEAT_PRODUCERDATA_H_
