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
#ifndef ROCKETMQ_PROTOCOL_HEARTBEAT_SUBSCRIPTIONDATA_HPP_
#define ROCKETMQ_PROTOCOL_HEARTBEAT_SUBSCRIPTIONDATA_HPP_

#include <cstdint>  // int64_t

#include <string>  // std::string
#include <vector>  // std::vector

#include <json/json.h>

#include "ExpressionType.h"
#include "UtilAll.h"

namespace rocketmq {

class SubscriptionData {
 public:
  SubscriptionData() : sub_version_(UtilAll::currentTimeMillis()), expression_type_(ExpressionType::TAG) {}
  SubscriptionData(const std::string& topic, const std::string& subString)
      : topic_(topic),
        sub_string_(subString),
        sub_version_(UtilAll::currentTimeMillis()),
        expression_type_(ExpressionType::TAG) {}

  SubscriptionData(const SubscriptionData& other) {
    sub_string_ = other.sub_string_;
    sub_version_ = other.sub_version_;
    tag_set_ = other.tag_set_;
    topic_ = other.topic_;
    code_set_ = other.code_set_;
    expression_type_ = other.expression_type_;
  }

  virtual ~SubscriptionData() = default;

  bool operator==(const SubscriptionData& other) const {
    // FIXME: tags
    return expression_type_ == expression_type_ && topic_ == other.topic_ && sub_string_ == other.sub_string_ &&
           sub_version_ == other.sub_version_ && tag_set_.size() == other.tag_set_.size();
  }
  bool operator!=(const SubscriptionData& other) const { return !operator==(other); }

  bool operator<(const SubscriptionData& other) const {
    int ret = topic_.compare(other.topic_);
    if (ret == 0) {
      return sub_string_.compare(other.sub_string_) < 0;
    } else {
      return ret < 0;
    }
  }

  inline bool containsTag(const std::string& tag) const {
    return std::find(tag_set_.begin(), tag_set_.end(), tag) != tag_set_.end();
  }

  Json::Value toJson() const {
    Json::Value root;
    root["topic"] = topic_;
    root["subString"] = sub_string_;
    root["subVersion"] = UtilAll::to_string(sub_version_);

    for (const auto& tag : tag_set_) {
      root["tagsSet"].append(tag);
    }

    for (const auto& code : code_set_) {
      root["codeSet"].append(code);
    }

    return root;
  }

 public:
  inline const std::string& topic() const { return topic_; }

  inline const std::string& sub_string() const { return sub_string_; }
  inline void set_sub_string(const std::string& sub) { sub_string_ = sub; }

  inline int64_t sub_version() const { return sub_version_; }

  inline std::vector<std::string>& tags_set() { return tag_set_; }

  inline std::vector<int32_t>& code_set() { return code_set_; }

  inline const std::string& expression_type() { return expression_type_; }

 private:
  std::string topic_;
  std::string sub_string_;
  int64_t sub_version_;
  std::vector<std::string> tag_set_;
  std::vector<int32_t> code_set_;
  std::string expression_type_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_HEARTBEAT_SUBSCRIPTIONDATA_HPP_
