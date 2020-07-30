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
#ifndef ROCKETMQ_PROTOCOL_TOPICROUTEDATA_HPP_
#define ROCKETMQ_PROTOCOL_TOPICROUTEDATA_HPP_

#include <json/json.h>

#include <algorithm>
#include <cstdlib>
#include <memory>

#include "ByteArray.h"
#include "Logging.h"
#include "RemotingSerializable.h"
#include "UtilAll.h"

namespace rocketmq {

class QueueData {
 public:
  QueueData() : read_queue_nums_(16), write_queue_nums_(16), perm_(6) {}
  QueueData(const std::string& broker_name, int read_queue_nums, int write_queue_nums, int perm)
      : broker_name_(broker_name),
        read_queue_nums_(read_queue_nums),
        write_queue_nums_(write_queue_nums),
        perm_(perm) {}
  QueueData(std::string&& broker_name, int read_queue_nums, int write_queue_nums, int perm)
      : broker_name_(std::move(broker_name)),
        read_queue_nums_(read_queue_nums),
        write_queue_nums_(write_queue_nums),
        perm_(perm) {}

  bool operator<(const QueueData& other) const { return broker_name_ < other.broker_name_; }

  bool operator==(const QueueData& other) const {
    return broker_name_ == other.broker_name_ && read_queue_nums_ == other.read_queue_nums_ &&
           write_queue_nums_ == other.write_queue_nums_ && perm_ == other.perm_;
  }
  bool operator!=(const QueueData& other) const { return !operator==(other); }

 public:
  inline const std::string& broker_name() const { return broker_name_; }
  inline void broker_name(const std::string& broker_name) { broker_name_ = broker_name; }

  inline int read_queue_nums() const { return read_queue_nums_; }
  inline void set_read_queue_nums(int read_queue_nums) { read_queue_nums_ = read_queue_nums; }

  inline int write_queue_nums() const { return write_queue_nums_; }
  inline void set_write_queue_nums(int write_queue_nums) { write_queue_nums_ = write_queue_nums; }

  inline int perm() const { return perm_; }
  inline void set_perm(int perm) { perm_ = perm; }

 private:
  std::string broker_name_;
  int read_queue_nums_;
  int write_queue_nums_;
  int perm_;
};

class BrokerData {
 public:
  BrokerData();
  BrokerData(const std::string& broker_name) : broker_name_(broker_name) {}
  BrokerData(std::string&& broker_name) : broker_name_(std::move(broker_name)) {}
  BrokerData(const std::string& broker_name, const std::map<int, std::string>& broker_addrs)
      : broker_name_(broker_name), broker_addrs_(broker_addrs) {}
  BrokerData(std::string&& broker_name, std::map<int, std::string>&& broker_addrs)
      : broker_name_(std::move(broker_name)), broker_addrs_(std::move(broker_addrs)) {}

  bool operator<(const BrokerData& other) const { return broker_name_ < other.broker_name_; }

  bool operator==(const BrokerData& other) const {
    return broker_name_ == other.broker_name_ && broker_addrs_ == other.broker_addrs_;
  }
  bool operator!=(const BrokerData& other) const { return !operator==(other); }

 public:
  inline const std::string& broker_name() const { return broker_name_; }
  inline void broker_name(const std::string& broker_name) { broker_name_ = broker_name; }

  inline const std::map<int, std::string>& broker_addrs() const { return broker_addrs_; }
  inline std::map<int, std::string>& broker_addrs() { return broker_addrs_; }

 private:
  std::string broker_name_;
  std::map<int, std::string> broker_addrs_;  // master:0; slave:1,2,3,etc.
};

class TopicRouteData;
typedef std::shared_ptr<TopicRouteData> TopicRouteDataPtr;

class TopicRouteData {
 public:
  static TopicRouteData* Decode(const ByteArray& bodyData) {
    Json::Value root = RemotingSerializable::fromJson(bodyData);

    std::unique_ptr<TopicRouteData> trd(new TopicRouteData());
    trd->set_order_topic_conf(root["orderTopicConf"].asString());

    auto& qds = root["queueDatas"];
    for (auto qd : qds) {
      trd->queue_datas().emplace_back(qd["brokerName"].asString(), qd["readQueueNums"].asInt(),
                                      qd["writeQueueNums"].asInt(), qd["perm"].asInt());
    }
    sort(trd->queue_datas().begin(), trd->queue_datas().end());

    auto& bds = root["brokerDatas"];
    for (auto bd : bds) {
      std::string broker_name = bd["brokerName"].asString();
      LOG_DEBUG_NEW("brokerName:{}", broker_name);
      auto& bas = bd["brokerAddrs"];
      Json::Value::Members members = bas.getMemberNames();
      std::map<int, std::string> broker_addrs;
      for (const auto& member : members) {
        int id = std::stoi(member);
        std::string addr = bas[member].asString();
        broker_addrs.emplace(id, std::move(addr));
        LOG_DEBUG_NEW("brokerId:{}, brokerAddr:{}", id, addr);
      }
      trd->broker_datas().emplace_back(std::move(broker_name), std::move(broker_addrs));
    }
    sort(trd->broker_datas().begin(), trd->broker_datas().end());

    return trd.release();
  }

  /**
   * Selects a (preferably master) broker address from the registered list.
   * If the master's address cannot be found, a slave broker address is selected in a random manner.
   *
   * @return Broker address.
   */
  std::string selectBrokerAddr() {
    auto bdSize = broker_datas_.size();
    if (bdSize > 0) {
      auto bdIndex = std::rand() % bdSize;
      const auto& bd = broker_datas_[bdIndex];
      const auto& broker_addrs = bd.broker_addrs();
      auto it = broker_addrs.find(MASTER_ID);
      if (it == broker_addrs.end()) {
        auto baSize = broker_addrs.size();
        auto baIndex = std::rand() % baSize;
        for (it = broker_addrs.begin(); baIndex > 0; baIndex--) {
          it++;
        }
      }
      return it->second;
    }
    return null;
  }

  bool operator==(const TopicRouteData& other) const {
    return broker_datas_ == other.broker_datas_ && order_topic_conf_ == other.order_topic_conf_ &&
           queue_datas_ == other.queue_datas_;
  }
  bool operator!=(const TopicRouteData& other) const { return !operator==(other); }

 public:
  inline const std::string& order_topic_conf() const { return order_topic_conf_; }
  inline void set_order_topic_conf(const std::string& orderTopicConf) { order_topic_conf_ = orderTopicConf; }

  inline std::vector<QueueData>& queue_datas() { return queue_datas_; }

  inline std::vector<BrokerData>& broker_datas() { return broker_datas_; }

 private:
  std::string order_topic_conf_;
  std::vector<QueueData> queue_datas_;
  std::vector<BrokerData> broker_datas_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_TOPICROUTEDATA_HPP_
