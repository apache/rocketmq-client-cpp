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
#ifndef ROCKETMQ_SUBSCRIPTIONDATA_H_
#define ROCKETMQ_SUBSCRIPTIONDATA_H_

#include <json/json.h>

#include <string>
#include <vector>

#include "RocketMQClient.h"

namespace rocketmq {

class ROCKETMQCLIENT_API SubscriptionData {
 public:
  SubscriptionData();
  SubscriptionData(const std::string& topic, const std::string& subString);
  SubscriptionData(const SubscriptionData& other);

  virtual ~SubscriptionData() = default;

  bool operator==(const SubscriptionData& other) const;
  bool operator!=(const SubscriptionData& other) const { return !operator==(other); }

  bool operator<(const SubscriptionData& other) const;

  Json::Value toJson() const;

 public:
  inline const std::string& topic() const { return topic_; }

  inline const std::string& sub_string() const { return sub_string_; }
  inline void set_sub_string(const std::string& sub) { sub_string_ = sub; }

  inline int64_t sub_version() const { return sub_version_; }

  inline std::vector<std::string>& tags_set() { return tag_set_; }

  inline void put_tag(const std::string& tag) { tag_set_.push_back(tag); }

  inline bool contain_tag(const std::string& tag) const {
    return std::find(tag_set_.begin(), tag_set_.end(), tag) != tag_set_.end();
  }

  inline void put_code(int32_t code) { code_set_.push_back(code); }

 private:
  std::string topic_;
  std::string sub_string_;
  int64_t sub_version_;
  std::vector<std::string> tag_set_;
  std::vector<int32_t> code_set_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_SUBSCRIPTIONDATA_H_
