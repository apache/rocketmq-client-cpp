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
#ifndef __SUB_SCRIPTION_DATA_H__
#define __SUB_SCRIPTION_DATA_H__

#include <string>

#include "UtilAll.h"
#include "json/json.h"

namespace rocketmq {
//<!************************************************************************
class SubscriptionData {
 public:
  SubscriptionData();
  virtual ~SubscriptionData() {
    m_tagSet.clear();
    m_codeSet.clear();
  }
  SubscriptionData(const std::string& topic, const std::string& subString);
  SubscriptionData(const SubscriptionData& other);

  const std::string& getTopic() const;
  const std::string& getSubString() const;
  void setSubString(const std::string& sub);
  int64 getSubVersion() const;

  void putTagsSet(const std::string& tag);
  bool containTag(const std::string& tag);
  std::vector<std::string>& getTagsSet();

  void putCodeSet(const std::string& tag);

  bool operator==(const SubscriptionData& other) const;
  bool operator<(const SubscriptionData& other) const;

  Json::Value toJson() const;

 private:
  std::string m_topic;
  std::string m_subString;
  int64 m_subVersion;
  std::vector<std::string> m_tagSet;
  std::vector<int> m_codeSet;
};

}  // namespace rocketmq

#endif  // __SUB_SCRIPTION_DATA_H__
