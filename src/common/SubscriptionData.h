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
#ifndef __SUBSCRIPTION_DATA_H__
#define __SUBSCRIPTION_DATA_H__

#include <string>
#include <vector>

#include <json/json.h>

#include "RocketMQClient.h"

namespace rocketmq {

class SubscriptionData;
typedef SubscriptionData* SubscriptionDataPtr;

class ROCKETMQCLIENT_API SubscriptionData {
 public:
  SubscriptionData();
  SubscriptionData(const std::string& topic, const std::string& subString);
  SubscriptionData(const SubscriptionData& other);

  virtual ~SubscriptionData() {
    m_tagSet.clear();
    m_codeSet.clear();
  }

  const std::string& getTopic() const;

  const std::string& getSubString() const;
  void setSubString(const std::string& sub);

  int64_t getSubVersion() const;

  void putTagsSet(const std::string& tag);
  bool containTag(const std::string& tag);
  std::vector<std::string>& getTagsSet();

  void putCodeSet(const int32 code);

  bool operator==(const SubscriptionData& other) const;
  bool operator<(const SubscriptionData& other) const;

  Json::Value toJson() const;

 private:
  std::string m_topic;
  std::string m_subString;
  int64_t m_subVersion;
  std::vector<std::string> m_tagSet;
  std::vector<int32_t> m_codeSet;
};

}  // namespace rocketmq

#endif  // __SUBSCRIPTION_DATA_H__
