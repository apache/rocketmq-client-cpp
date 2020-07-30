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
#ifndef ROCKETMQ_PROTOCOL_CONSUMERRUNNINGINFO_H_
#define ROCKETMQ_PROTOCOL_CONSUMERRUNNINGINFO_H_

#include "MessageQueue.hpp"
#include "ProcessQueueInfo.hpp"
#include "protocol/heartbeat/SubscriptionData.hpp"

namespace rocketmq {

class ConsumerRunningInfo {
 public:
  static const std::string PROP_NAMESERVER_ADDR;
  static const std::string PROP_THREADPOOL_CORE_SIZE;
  static const std::string PROP_CONSUME_ORDERLY;
  static const std::string PROP_CONSUME_TYPE;
  static const std::string PROP_CLIENT_VERSION;
  static const std::string PROP_CONSUMER_START_TIMESTAMP;

 public:
  ConsumerRunningInfo() {}
  virtual ~ConsumerRunningInfo() {
    properties_.clear();
    mq_table_.clear();
    subscription_set_.clear();
  }

  std::string encode();

 public:
  inline const std::map<std::string, std::string> getProperties() const { return properties_; }
  inline void setProperties(const std::map<std::string, std::string>& properties) { properties_ = properties; }
  inline void setProperty(const std::string& key, const std::string& value) { properties_[key] = value; }

  inline const std::map<MQMessageQueue, ProcessQueueInfo> getMqTable() const { return mq_table_; }
  inline void setMqTable(const MQMessageQueue& queue, ProcessQueueInfo queueInfo) { mq_table_[queue] = queueInfo; }

  // const std::map<std::string, ConsumeStatus> getStatusTable() const;
  // void setStatusTable(const std::map<std::string, ConsumeStatus>& statusTable) ;

  inline const std::vector<SubscriptionData> getSubscriptionSet() const { return subscription_set_; }
  inline void setSubscriptionSet(const std::vector<SubscriptionData>& subscriptionSet) {
    subscription_set_ = subscriptionSet;
  }

  inline const std::string getJstack() const { return jstack_; }
  inline void setJstack(const std::string& jstack) { this->jstack_ = jstack; }

 private:
  std::map<std::string, std::string> properties_;
  std::vector<SubscriptionData> subscription_set_;
  std::map<MQMessageQueue, ProcessQueueInfo> mq_table_;
  // std::map<std::string, ConsumeStatus> statusTable;
  std::string jstack_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_CONSUMERRUNNINGINFO_H_
