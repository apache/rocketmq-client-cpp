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
#include "SubscriptionData.h"

#include <algorithm>
#include <sstream>
#include <vector>

#include "Logging.h"
#include "UtilAll.h"

namespace rocketmq {

SubscriptionData::SubscriptionData() {
  sub_version_ = UtilAll::currentTimeMillis();
}

SubscriptionData::SubscriptionData(const std::string& topic, const std::string& subString)
    : topic_(topic), sub_string_(subString) {
  sub_version_ = UtilAll::currentTimeMillis();
}

SubscriptionData::SubscriptionData(const SubscriptionData& other) {
  sub_string_ = other.sub_string_;
  sub_version_ = other.sub_version_;
  tag_set_ = other.tag_set_;
  topic_ = other.topic_;
  code_set_ = other.code_set_;
}

bool SubscriptionData::operator==(const SubscriptionData& other) const {
  if (topic_ != other.topic_) {
    return false;
  }
  if (sub_string_ != other.sub_string_) {
    return false;
  }
  if (sub_version_ != other.sub_version_) {
    return false;
  }
  if (tag_set_.size() != other.tag_set_.size()) {
    return false;
  }
  return true;
}

bool SubscriptionData::operator<(const SubscriptionData& other) const {
  int ret = topic_.compare(other.topic_);
  if (ret == 0) {
    return sub_string_.compare(other.sub_string_) < 0;
  } else {
    return ret < 0;
  }
}

Json::Value SubscriptionData::toJson() const {
  Json::Value outJson;
  outJson["topic"] = topic_;
  outJson["subString"] = sub_string_;
  outJson["subVersion"] = UtilAll::to_string(sub_version_);

  for (const auto& tag : tag_set_) {
    outJson["tagsSet"].append(tag);
  }

  for (const auto& code : code_set_) {
    outJson["codeSet"].append(code);
  }

  return outJson;
}

}  // namespace rocketmq
