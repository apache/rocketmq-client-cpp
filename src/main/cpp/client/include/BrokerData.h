#pragma once

#include "RpcClient.h"
#include "apache/rocketmq/v1/definition.pb.h"

#include <map>
#include <string>
#include <utility>

namespace rocketmq {

namespace rmq = apache::rocketmq::v1;

class BrokerData {
public:
  explicit BrokerData(const rmq::BrokerData& broker_data)
      : cluster_(broker_data.cluster()), broker_name_(broker_data.broker_name()),
        debug_string_(broker_data.DebugString()) {
    for (const auto& entry : broker_data.addresses()) {
      addresses_.insert(std::make_pair(entry.first, entry.second));
    }
  }

  bool operator==(const BrokerData& rhs) const {
    return cluster_ == rhs.cluster_ && broker_name_ == rhs.broker_name_ && addresses_ == rhs.addresses_;
  }

  bool operator<(const BrokerData& rhs) const {
    if (cluster_ != rhs.cluster_) {
      return cluster_ < rhs.cluster_;
    }

    return broker_name_ < rhs.broker_name_;
  }

  const std::string& cluster() const { return cluster_; }

  const std::string& brokerName() const { return broker_name_; }

  const std::map<int64_t, std::string>& addresses() const { return addresses_; }

  const std::string& debugString() const { return debug_string_; }

private:
  std::string cluster_;
  std::string broker_name_;
  std::map<int64_t, std::string> addresses_;

  std::string debug_string_;
};

} // namespace rocketmq
