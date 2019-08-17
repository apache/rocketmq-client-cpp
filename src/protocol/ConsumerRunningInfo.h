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
#ifndef __CONSUMER_RUNNING_INFO_H__
#define __CONSUMER_RUNNING_INFO_H__

#include "MessageQueue.h"
#include "ProcessQueueInfo.h"
#include "SubscriptionData.h"

namespace rocketmq {

class ConsumerRunningInfo {
 public:
  ConsumerRunningInfo() {}
  virtual ~ConsumerRunningInfo() {
    properties.clear();
    mqTable.clear();
    subscriptionSet.clear();
  }

 public:
  static const std::string PROP_NAMESERVER_ADDR;
  static const std::string PROP_THREADPOOL_CORE_SIZE;
  static const std::string PROP_CONSUME_ORDERLY;
  static const std::string PROP_CONSUME_TYPE;
  static const std::string PROP_CLIENT_VERSION;
  static const std::string PROP_CONSUMER_START_TIMESTAMP;

 public:
  const std::map<std::string, std::string> getProperties() const;
  void setProperties(const std::map<std::string, std::string>& input_properties);
  void setProperty(const std::string& key, const std::string& value);
  const std::map<MessageQueue, ProcessQueueInfo> getMqTable() const;
  void setMqTable(MessageQueue queue, ProcessQueueInfo queueInfo);
  // const map<string, ConsumeStatus> getStatusTable() const;
  // void setStatusTable(const map<string, ConsumeStatus>& input_statusTable) ;
  const std::vector<SubscriptionData> getSubscriptionSet() const;
  void setSubscriptionSet(const std::vector<SubscriptionData>& input_subscriptionSet);
  const std::string getJstack() const;
  void setJstack(const std::string& input_jstack);
  std::string encode();

 private:
  std::map<std::string, std::string> properties;
  std::vector<SubscriptionData> subscriptionSet;
  std::map<MessageQueue, ProcessQueueInfo> mqTable;
  // map<string, ConsumeStatus> statusTable;
  std::string jstack;
};

}  // namespace rocketmq

#endif  // __CONSUMER_RUNNING_INFO_H__
