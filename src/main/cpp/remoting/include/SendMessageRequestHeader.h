#pragma once

#include <cstdint>

#include "CommandCustomHeader.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class SendMessageRequestHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value& root) const override;

  void producerGroup(absl::string_view producer_group) {
    producer_group_ = std::string(producer_group.data(), producer_group.length());
  }

  void topic(absl::string_view topic) {
    topic_ = std::string(topic.data(), topic.length());
  }

  void queueId(std::int32_t queue_id) {
    queue_id_ = queue_id;
  }

  void systemFlag(std::int32_t system_flag) {
    system_flag_ = system_flag;
  }

  void flag(std::int32_t flag) {
    flag_ = flag;
  }

  void properties(absl::string_view properties) {
    properties_ = std::string(properties.data(), properties.length());
  }

private:
  std::string producer_group_;
  std::string topic_;
  std::string default_topic_{"TBW102"};
  std::int32_t default_topic_queue_number_{8};
  std::int32_t queue_id_{-1};
  std::int32_t system_flag_{0};
  std::int64_t born_timestamp_{absl::ToInt64Milliseconds(absl::Now() - absl::UnixEpoch())};
  std::int32_t flag_{0};
  std::string properties_;
  std::int32_t reconsume_times_{0};
  bool unit_mode_{false};
  bool batch_{false};
  std::int32_t max_reconsume_times_{16};
};

ROCKETMQ_NAMESPACE_END