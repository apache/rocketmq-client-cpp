#pragma once

#include <cstdint>
#include <string>

#include "absl/time/clock.h"
#include "absl/time/time.h"

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class PopMessageRequestHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value& root) const override;

  void consumerGroup(absl::string_view consumer_group) {
    consumer_group_ = std::string(consumer_group.data(), consumer_group.length());
  }

  void topic(absl::string_view topic) {
    topic_ = std::string(topic.data(), topic.length());
  }

  void queueId(std::int32_t queue_id) {
    queue_id_ = queue_id;
  }

  void expressionType(absl::string_view type) {
    expression_type_ = std::string(type.data(), type.length());
  }

  void expression(absl::string_view expression) {
    expression_ = std::string(expression.data(), expression.length());
  }

  void batchSize(std::int32_t batch_size) {
    max_message_number_ = batch_size;
  }

  void invisibleTime(std::int64_t time) {
    invisible_time_ = time;
  }

  void generateMessageHandle(bool generate) {
    generate_message_handle_ = generate;
  }

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
  bool generate_message_handle_{true};
};

ROCKETMQ_NAMESPACE_END