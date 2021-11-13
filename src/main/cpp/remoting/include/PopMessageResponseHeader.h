#pragma once

#include <cstdint>
#include <string>

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class PopMessageResponseHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value &root) const override {}

  static PopMessageResponseHeader *decode(const google::protobuf::Value &root);

  std::int64_t invisibleTimeInMillis() const { return invisible_time_; }

  std::int64_t popTime() const { return pop_time_; }

  const std::string &startOffsetInfo() const { return start_offset_info_; }

  const std::string &messageOffsetInfo() const { return message_offset_info_; }

  const std::string &orderCountInfo() const { return order_count_info_; }

private:
  std::int64_t pop_time_{0};
  std::int64_t invisible_time_{0};
  std::int32_t revive_queue_id_{0};
  std::int64_t rest_number_{0};
  std::string start_offset_info_;
  std::string message_offset_info_;
  std::string order_count_info_;
};

ROCKETMQ_NAMESPACE_END
