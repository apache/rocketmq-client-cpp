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
#ifndef ROCKETMQ_PROTOCOL_HEARTBEAT_CONSUMERDATA_H_
#define ROCKETMQ_PROTOCOL_HEARTBEAT_CONSUMERDATA_H_

#include <algorithm>  // std::move
#include <string>     // std::string
#include <vector>     // std::vector

#include <json/json.h>

#include "ConsumeType.h"
#include "SubscriptionData.hpp"

namespace rocketmq {

class ConsumerData {
 public:
  ConsumerData(const std::string& group_name,
               ConsumeType consume_type,
               MessageModel message_model,
               ConsumeFromWhere consume_from_where,
               std::vector<SubscriptionData>&& subscription_data_set)
      : group_name_(group_name),
        consume_type_(consume_type),
        message_model_(message_model),
        consume_from_where_(consume_from_where),
        subscription_data_set_(std::move(subscription_data_set)) {}

  bool operator<(const ConsumerData& other) const { return group_name_ < other.group_name_; }

  Json::Value toJson() const {
    Json::Value root;
    root["groupName"] = group_name_;
    root["consumeType"] = consume_type_;
    root["messageModel"] = message_model_;
    root["consumeFromWhere"] = consume_from_where_;

    for (const auto& sd : subscription_data_set_) {
      root["subscriptionDataSet"].append(sd.toJson());
    }

    return root;
  }

 public:
  inline const std::string& group_name() const { return group_name_; }
  inline void set_group_name(const std::string& group_name) { group_name_ = group_name; }

  inline ConsumeType consume_type() const { return consume_type_; }
  inline void set_consume_type(ConsumeType consume_type) { consume_type_ = consume_type; }

  inline MessageModel message_model() const { return message_model_; }

  inline ConsumeFromWhere consume_from_where() const { return consume_from_where_; }

  inline std::vector<SubscriptionData> subscription_data_set() { return subscription_data_set_; }

 private:
  std::string group_name_;
  ConsumeType consume_type_;
  MessageModel message_model_;
  ConsumeFromWhere consume_from_where_;
  std::vector<SubscriptionData> subscription_data_set_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_HEARTBEAT_CONSUMERDATA_H_
