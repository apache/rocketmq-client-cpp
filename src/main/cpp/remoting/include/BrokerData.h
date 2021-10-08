#pragma once

#include <cstdint>
#include <string>

#include "absl/container/flat_hash_map.h"
#include "google/protobuf/struct.pb.h"
#include "google/protobuf/util/json_util.h"

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

struct BrokerData {
  std::string cluster_;
  std::string broker_name_;
  absl::flat_hash_map<std::int64_t, std::string> broker_addresses_;

  static BrokerData decode(const google::protobuf::Struct& root);
};

ROCKETMQ_NAMESPACE_END