#pragma once

#include <cstdint>

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class AckMessageRequestHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value& root) const override;

private:
  std::string consumer_group_;
  std::string topic_;
  std::int32_t queue_id_{0};
  std::string extra_info_;
  std::int64_t offset_{0};
};

ROCKETMQ_NAMESPACE_END