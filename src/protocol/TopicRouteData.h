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
#ifndef __TOPICROUTEDATA_H__
#define __TOPICROUTEDATA_H__
#include <algorithm>
#include <cstdlib>
#include "Logging.h"
#include "UtilAll.h"
#include "dataBlock.h"
#include "json/json.h"

namespace rocketmq {

//<!***************************************************************************
struct QueueData {
  std::string brokerName;
  int readQueueNums;
  int writeQueueNums;
  int perm;

  bool operator<(const QueueData& other) const { return brokerName < other.brokerName; }

  bool operator==(const QueueData& other) const {
    return brokerName == other.brokerName && readQueueNums == other.readQueueNums &&
           writeQueueNums == other.writeQueueNums && perm == other.perm;
  }
};

//<!***************************************************************************
struct BrokerData {
  std::string brokerName;
  std::map<int, string> brokerAddrs;  //<!0:master,1,2.. slave

  bool operator<(const BrokerData& other) const { return brokerName < other.brokerName; }

  bool operator==(const BrokerData& other) const {
    return brokerName == other.brokerName && brokerAddrs == other.brokerAddrs;
  }
};

//<!************************************************************************/
class TopicRouteData {
 public:
  virtual ~TopicRouteData() {
    m_brokerDatas.clear();
    m_queueDatas.clear();
  }

  static TopicRouteData* Decode(const MemoryBlock* mem) {
    //<!see doc/TopicRouteData.json;
    const char* const pData = static_cast<const char*>(mem->getData());
    string data(pData, mem->getSize());

    Json::CharReaderBuilder charReaderBuilder;
    charReaderBuilder.settings_["allowNumericKeys"] = true;
    unique_ptr<Json::CharReader> pCharReaderPtr(charReaderBuilder.newCharReader());

    const char* begin = pData;
    const char* end = pData + mem->getSize();
    Json::Value root;
    string errs;

    if (!pCharReaderPtr->parse(begin, end, &root, &errs)) {
      LOG_ERROR("parse json error:%s, value isArray:%d, isObject:%d", errs.c_str(), root.isArray(), root.isObject());
      return nullptr;
    }

    auto* trd = new TopicRouteData();
    trd->setOrderTopicConf(root["orderTopicConf"].asString());

    Json::Value qds = root["queueDatas"];
    for (auto qd : qds) {
      QueueData d;
      d.brokerName = qd["brokerName"].asString();
      d.readQueueNums = qd["readQueueNums"].asInt();
      d.writeQueueNums = qd["writeQueueNums"].asInt();
      d.perm = qd["perm"].asInt();
      trd->getQueueDatas().push_back(d);
    }
    sort(trd->getQueueDatas().begin(), trd->getQueueDatas().end());

    Json::Value bds = root["brokerDatas"];
    for (auto bd : bds) {
      BrokerData d;
      d.brokerName = bd["brokerName"].asString();
      LOG_DEBUG("brokerName:%s", d.brokerName.c_str());
      Json::Value bas = bd["brokerAddrs"];
      Json::Value::Members mbs = bas.getMemberNames();
      for (const auto& key : mbs) {
        int id = atoi(key.c_str());
        string addr = bas[key].asString();
        d.brokerAddrs[id] = addr;
        LOG_DEBUG("brokerId:%d, brokerAddr:%s", id, addr.c_str());
      }
      trd->getBrokerDatas().push_back(d);
    }
    sort(trd->getBrokerDatas().begin(), trd->getBrokerDatas().end());

    return trd;
  }

  /**
   * Selects a (preferably master) broker address from the registered list.
   * If the master's address cannot be found, a slave broker address is selected in a random manner.
   *
   * @return Broker address.
   */
  std::string selectBrokerAddr() {
    int bdSize = m_brokerDatas.size();
    if (bdSize > 0) {
      int bdIndex = std::rand() % bdSize;
      auto bd = m_brokerDatas[bdIndex];
      auto iter = bd.brokerAddrs.find(MASTER_ID);
      if (iter == bd.brokerAddrs.end()) {
        int baSize = bd.brokerAddrs.size();
        int baIndex = std::rand() % baSize;
        iter = bd.brokerAddrs.begin();
        for (; baIndex > 0; baIndex--) {
          iter++;
        }
      }
      return iter->second;
    }
    return "";
  }

  std::vector<QueueData>& getQueueDatas() { return m_queueDatas; }

  std::vector<BrokerData>& getBrokerDatas() { return m_brokerDatas; }

  const std::string& getOrderTopicConf() const { return m_orderTopicConf; }

  void setOrderTopicConf(const string& orderTopicConf) { m_orderTopicConf = orderTopicConf; }

  bool operator==(const TopicRouteData& other) const {
    return m_brokerDatas == other.m_brokerDatas && m_orderTopicConf == other.m_orderTopicConf &&
           m_queueDatas == other.m_queueDatas;
  }

 private:
  std::string m_orderTopicConf;
  std::vector<QueueData> m_queueDatas;
  std::vector<BrokerData> m_brokerDatas;
};

}  // namespace rocketmq

#endif
