#pragma once

#include <cstdint>

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class QueryConsumerOffsetResponseHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value& root) const override {
  }

  static QueryConsumerOffsetResponseHeader* decode(const google::protobuf::Value& root);

  std::int64_t offset_{-1};
};

ROCKETMQ_NAMESPACE_END
