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
  m_subVersion = UtilAll::currentTimeMillis();
}

SubscriptionData::SubscriptionData(const std::string& topic, const std::string& subString)
    : m_topic(topic), m_subString(subString) {
  m_subVersion = UtilAll::currentTimeMillis();
}

SubscriptionData::SubscriptionData(const SubscriptionData& other) {
  m_subString = other.m_subString;
  m_subVersion = other.m_subVersion;
  m_tagSet = other.m_tagSet;
  m_topic = other.m_topic;
  m_codeSet = other.m_codeSet;
}

const std::string& SubscriptionData::getTopic() const {
  return m_topic;
}

const std::string& SubscriptionData::getSubString() const {
  return m_subString;
}

void SubscriptionData::setSubString(const std::string& sub) {
  m_subString = sub;
}

int64_t SubscriptionData::getSubVersion() const {
  return m_subVersion;
}

void SubscriptionData::putTagsSet(const std::string& tag) {
  m_tagSet.push_back(tag);
}

bool SubscriptionData::containTag(const std::string& tag) {
  return std::find(m_tagSet.begin(), m_tagSet.end(), tag) != m_tagSet.end();
}

std::vector<std::string>& SubscriptionData::getTagsSet() {
  return m_tagSet;
}

void SubscriptionData::putCodeSet(int32_t code) {
  m_codeSet.push_back(code);
}

bool SubscriptionData::operator==(const SubscriptionData& other) const {
  if (!m_topic.compare(other.m_topic)) {
    return false;
  }
  if (!m_subString.compare(other.m_subString)) {
    return false;
  }
  if (m_subVersion != other.m_subVersion) {
    return false;
  }
  if (m_tagSet.size() != other.m_tagSet.size()) {
    return false;
  }
  return true;
}

bool SubscriptionData::operator<(const SubscriptionData& other) const {
  int ret = m_topic.compare(other.m_topic);
  if (ret == 0) {
    return m_subString.compare(other.m_subString) < 0;
  } else {
    return ret < 0;
  }
}

Json::Value SubscriptionData::toJson() const {
  Json::Value outJson;
  outJson["topic"] = m_topic;
  outJson["subString"] = m_subString;
  outJson["subVersion"] = UtilAll::to_string(m_subVersion);

  for (const auto& tag : m_tagSet) {
    outJson["tagsSet"].append(tag);
  }

  for (const auto& code : m_codeSet) {
    outJson["codeSet"].append(code);
  }

  return outJson;
}

}  // namespace rocketmq
