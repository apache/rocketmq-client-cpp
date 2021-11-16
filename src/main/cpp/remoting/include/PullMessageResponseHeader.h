#pragma once

#include <cstdint>

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class PullMessageResponseHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value& root) const override {
  }

  static PullMessageResponseHeader* decode(const google::protobuf::Value& root);

  std::int64_t suggest_which_broker_id_{0};
  std::int64_t next_begin_offset_{0};
  std::int64_t min_offset_{0};
  std::int64_t max_offset_{0};
  std::int32_t topic_system_flag_{0};
  std::int32_t group_system_flag_{0};
};

ROCKETMQ_NAMESPACE_END