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

#ifndef __HEARTBEATDATA_H__
#define __HEARTBEATDATA_H__

#include <cstdlib>
#include <mutex>
#include <string>
#include <vector>

#include "ConsumeType.h"
#include "SubscriptionData.h"

namespace rocketmq {

//<!***************************************************************************
class ProducerData {
 public:
  ProducerData(){};
  bool operator<(const ProducerData& pd) const { return groupName < pd.groupName; }
  Json::Value toJson() const {
    Json::Value outJson;
    outJson["groupName"] = groupName;
    return outJson;
  }

 public:
  std::string groupName;
};

//<!***************************************************************************
class ConsumerData {
 public:
  ConsumerData(){};
  virtual ~ConsumerData() { subscriptionDataSet.clear(); }
  bool operator<(const ConsumerData& cd) const { return groupName < cd.groupName; }

  Json::Value toJson() const {
    Json::Value outJson;
    outJson["groupName"] = groupName;
    outJson["consumeFromWhere"] = consumeFromWhere;
    outJson["consumeType"] = consumeType;
    outJson["messageModel"] = messageModel;

    for (const auto& sd : subscriptionDataSet) {
      outJson["subscriptionDataSet"].append(sd.toJson());
    }

    return outJson;
  }

 public:
  std::string groupName;
  ConsumeType consumeType;
  MessageModel messageModel;
  ConsumeFromWhere consumeFromWhere;
  std::vector<SubscriptionData> subscriptionDataSet;
};

//<!***************************************************************************
class HeartbeatData {
 public:
  virtual ~HeartbeatData() {
    m_producerDataSet.clear();
    m_consumerDataSet.clear();
  }
  void Encode(std::string& outData) {
    Json::Value root;

    // id;
    root["clientID"] = m_clientID;

    // consumer;
    {
      std::lock_guard<std::mutex> lock(m_consumerDataMutex);
      for (const auto& cd : m_consumerDataSet) {
        root["consumerDataSet"].append(cd.toJson());
      }
    }

    // producer;
    {
      std::lock_guard<std::mutex> lock(m_producerDataMutex);
      for (const auto& pd : m_producerDataSet) {
        root["producerDataSet"].append(pd.toJson());
      }
    }

    // output;
    Json::FastWriter fastwrite;
    outData = fastwrite.write(root);
  }

  void setClientID(const std::string& clientID) { m_clientID = clientID; }

  bool isProducerDataSetEmpty() {
    std::lock_guard<std::mutex> lock(m_producerDataMutex);
    return m_producerDataSet.empty();
  }

  void insertDataToProducerDataSet(ProducerData& producerData) {
    std::lock_guard<std::mutex> lock(m_producerDataMutex);
    m_producerDataSet.push_back(producerData);
  }

  bool isConsumerDataSetEmpty() {
    std::lock_guard<std::mutex> lock(m_consumerDataMutex);
    return m_consumerDataSet.empty();
  }

  void insertDataToConsumerDataSet(ConsumerData& consumerData) {
    std::lock_guard<std::mutex> lock(m_consumerDataMutex);
    m_consumerDataSet.push_back(consumerData);
  }

 private:
  std::string m_clientID;
  std::vector<ProducerData> m_producerDataSet;
  std::vector<ConsumerData> m_consumerDataSet;
  std::mutex m_producerDataMutex;
  std::mutex m_consumerDataMutex;
};

}  // namespace rocketmq

#endif
