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
#ifndef __REPLY_MESSAGE_REQUEST_HEADER_H__
#define __REPLY_MESSAGE_REQUEST_HEADER_H__

#include <vector>

#include "CommandCustomHeader.h"

namespace rocketmq {

class ReplyMessageRequestHeader : public CommandCustomHeader {
 public:
  static ReplyMessageRequestHeader* Decode(std::map<std::string, std::string>& extFields);

  const std::string& getProducerGroup() const;
  void setProducerGroup(const std::string& producerGroup);

  const std::string& getTopic() const;
  void setTopic(const std::string& topic);

  const std::string& getDefaultTopic() const;
  void setDefaultTopic(const std::string& defaultTopic);

  int32_t getDefaultTopicQueueNums() const;
  void setDefaultTopicQueueNums(int32_t defaultTopicQueueNums);

  int32_t getQueueId() const;
  void setQueueId(int32_t queueId);

  int32_t getSysFlag() const;
  void setSysFlag(int32_t sysFlag);

  int64_t getBornTimestamp() const;
  void setBornTimestamp(int64_t bornTimestamp);

  int32_t getFlag() const;
  void setFlag(int32_t flag);

  const std::string& getProperties() const;
  void setProperties(const std::string& properties);

  int32_t getReconsumeTimes() const;
  void setReconsumeTimes(int32_t reconsumeTimes);

  bool getUnitMode() const;
  void setUnitMode(bool unitMode);

  const std::string& getBornHost() const;
  void setBornHost(const std::string& bornHost);

  const std::string& getStoreHost() const;
  void setStoreHost(const std::string& storeHost);

  int64_t getStoreTimestamp() const;
  void setStoreTimestamp(int64_t stroeTimestamp);

 private:
  std::string producerGroup;
  std::string topic;
  std::string defaultTopic;
  int32_t defaultTopicQueueNums;
  int32_t queueId;
  int32_t sysFlag;
  int64_t bornTimestamp;
  int32_t flag;
  std::string properties;  // nullable
  int32_t reconsumeTimes;  // nullable
  bool unitMode;           // nullable

  std::string bornHost;
  std::string storeHost;
  int64_t storeTimestamp;
};

}  // namespace rocketmq

#endif  // __REPLY_MESSAGE_REQUEST_HEADER_H__
