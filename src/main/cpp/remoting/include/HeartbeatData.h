#pragma once

#include "ConsumerData.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

struct HeartbeatData {
  std::string client_id_;
  absl::flat_hash_set<ConsumerData> consumer_data_set_;

  void encode(google::protobuf::Struct& root) const;
};

ROCKETMQ_NAMESPACE_END