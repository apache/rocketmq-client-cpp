#pragma once

#include <cstdint>

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class SendMessageResponseHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value& root) const override {
  }

  static SendMessageResponseHeader* decode(const google::protobuf::Value& root);

private:
  std::string message_id_;
  std::int32_t queue_id_{0};
  std::int64_t queue_offset_{0};
  std::string transaction_id_;
};

ROCKETMQ_NAMESPACE_END