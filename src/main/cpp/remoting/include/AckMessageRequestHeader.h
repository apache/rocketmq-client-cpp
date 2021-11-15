#pragma once

#include <cstdint>

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class AckMessageRequestHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value &root) const override;

  void consumerGroup(const std::string &consumer_group) {
    consumer_group_ = consumer_group;
  }

  void topic(const std::string &topic) { topic_ = topic; }

  void queueId(std::int32_t queue_id) { queue_id_ = queue_id; }

  void receiptHandle(const std::string &receipt_handle) {
    extra_info_ = receipt_handle;
  }

  void offset(std::int64_t offset) { offset_ = offset; }

private:
  std::string consumer_group_;
  std::string topic_;
  std::int32_t queue_id_{0};
  std::string extra_info_;
  std::int64_t offset_{0};
};

ROCKETMQ_NAMESPACE_END