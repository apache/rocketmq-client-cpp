#pragma once

#include <cstdint>

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class PullMessageRequestHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value& root) const override;

  std::string consumer_group_;
  std::string topic_;
  std::int32_t queue_id_{0};
  std::int64_t queue_offset_{0};
  std::int32_t max_msg_number_{32};
  std::int32_t system_flag_{0};
  std::int64_t commit_offset_{0};
  std::int64_t suspend_timeout_millis_{30000};
  std::string subscription_;
  std::int64_t sub_version_{0};
  std::string expression_type_;
  std::string sub_properties_;
  std::string broker_name_;
};

ROCKETMQ_NAMESPACE_END