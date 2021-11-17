#pragma once

#include <cstdint>

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class UpdateConsumerOffsetResponseHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value& root) const override {
  }

  static UpdateConsumerOffsetResponseHeader* decode(const google::protobuf::Value& root);
};

ROCKETMQ_NAMESPACE_END
