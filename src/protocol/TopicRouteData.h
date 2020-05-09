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
#ifndef __TOPIC_ROUTE_DATA_H__
#define __TOPIC_ROUTE_DATA_H__

#include <json/json.h>

#include <algorithm>
#include <cstdlib>
#include <memory>

#include "DataBlock.h"
#include "Logging.h"
#include "RemotingSerializable.h"
#include "UtilAll.h"

namespace rocketmq {

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
  bool operator!=(const QueueData& other) const { return !operator==(other); }
};

struct BrokerData {
  std::string brokerName;
  std::map<int, std::string> brokerAddrs;  // master:0; slave:1,2,3,etc.

  bool operator<(const BrokerData& other) const { return brokerName < other.brokerName; }

  bool operator==(const BrokerData& other) const {
    return brokerName == other.brokerName && brokerAddrs == other.brokerAddrs;
  }
  bool operator!=(const BrokerData& other) const { return !operator==(other); }
};

class TopicRouteData;
typedef std::shared_ptr<TopicRouteData> TopicRouteDataPtr;

class TopicRouteData {
 public:
  static TopicRouteData* Decode(MemoryBlock& mem) {
    Json::Value root = RemotingSerializable::fromJson(mem);

    std::unique_ptr<TopicRouteData> trd(new TopicRouteData());
    trd->setOrderTopicConf(root["orderTopicConf"].asString());

    auto& qds = root["queueDatas"];
    for (auto qd : qds) {
      QueueData d;
      d.brokerName = qd["brokerName"].asString();
      d.readQueueNums = qd["readQueueNums"].asInt();
      d.writeQueueNums = qd["writeQueueNums"].asInt();
      d.perm = qd["perm"].asInt();
      trd->getQueueDatas().push_back(d);
    }
    sort(trd->getQueueDatas().begin(), trd->getQueueDatas().end());

    auto& bds = root["brokerDatas"];
    for (auto bd : bds) {
      BrokerData d;
      d.brokerName = bd["brokerName"].asString();
      LOG_DEBUG("brokerName:%s", d.brokerName.c_str());
      Json::Value bas = bd["brokerAddrs"];
      Json::Value::Members mbs = bas.getMemberNames();
      for (const auto& key : mbs) {
        int id = std::stoi(key);
        std::string addr = bas[key].asString();
        d.brokerAddrs[id] = addr;
        LOG_DEBUG("brokerId:%d, brokerAddr:%s", id, addr.c_str());
      }
      trd->getBrokerDatas().push_back(d);
    }
    sort(trd->getBrokerDatas().begin(), trd->getBrokerDatas().end());

    return trd.release();
  }

  /**
   * Selects a (preferably master) broker address from the registered list.
   * If the master's address cannot be found, a slave broker address is selected in a random manner.
   *
   * @return Broker address.
   */
  std::string selectBrokerAddr() {
    auto bdSize = m_brokerDatas.size();
    if (bdSize > 0) {
      auto bdIndex = std::rand() % bdSize;
      const auto& bd = m_brokerDatas[bdIndex];
      auto it = bd.brokerAddrs.find(MASTER_ID);
      if (it == bd.brokerAddrs.end()) {
        auto baSize = bd.brokerAddrs.size();
        auto baIndex = std::rand() % baSize;
        it = bd.brokerAddrs.begin();
        for (; baIndex > 0; baIndex--) {
          it++;
        }
      }
      return it->second;
    }
    return "";
  }

  std::vector<QueueData>& getQueueDatas() { return m_queueDatas; }

  std::vector<BrokerData>& getBrokerDatas() { return m_brokerDatas; }

  const std::string& getOrderTopicConf() const { return m_orderTopicConf; }

  void setOrderTopicConf(const std::string& orderTopicConf) { m_orderTopicConf = orderTopicConf; }

  bool operator==(const TopicRouteData& other) const {
    return m_brokerDatas == other.m_brokerDatas && m_orderTopicConf == other.m_orderTopicConf &&
           m_queueDatas == other.m_queueDatas;
  }
  bool operator!=(const TopicRouteData& other) const { return !operator==(other); }

 private:
  std::string m_orderTopicConf;
  std::vector<QueueData> m_queueDatas;
  std::vector<BrokerData> m_brokerDatas;
};

}  // namespace rocketmq

#endif  // __TOPIC_ROUTE_DATA_H__
