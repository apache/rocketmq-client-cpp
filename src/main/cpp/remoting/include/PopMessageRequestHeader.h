#pragma once

#include <cstdint>

#include "absl/time/clock.h"
#include "absl/time/time.h"

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class PopMessageRequestHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value& root) const override;

private:
  std::string consumer_group_;
  std::string topic_;
  std::int32_t queue_id_{0};
  std::int32_t max_message_number_{32};
  std::int64_t invisible_time_{30000};
  std::int64_t poll_time_{absl::ToInt64Milliseconds(absl::Now() - absl::UnixEpoch())};
  std::int64_t born_time_{absl::ToInt64Milliseconds(absl::Now() - absl::UnixEpoch())};
  std::int32_t init_mode_{0};
  std::string queue_id_list_string_;
  std::string expression_type_;
  std::string expression_;
  bool order_{false};
};

ROCKETMQ_NAMESPACE_END