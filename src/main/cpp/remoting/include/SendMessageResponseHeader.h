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

  const std::string& messageId() const {
    return message_id_;
  }

  void messageId(absl::string_view message_id) {
    message_id_ = std::string(message_id.data(), message_id.length());
  }

  std::int32_t queueId() const {
    return queue_id_;
  }

  void queueId(std::int32_t queue_id) {
    queue_id_ = queue_id;
  }

  std::int64_t queueOffset() const {
    return queue_offset_;
  }

  void queueOffset(std::int64_t queue_offset) {
    queue_offset_ = queue_offset;
  }

  const std::string& transactionId() const {
    return transaction_id_;
  }

  void transactionId(absl::string_view transaction_id) {
    transaction_id_ = std::string(transaction_id.data(), transaction_id.length());
  }

private:
  std::string message_id_;
  std::int32_t queue_id_{0};
  std::int64_t queue_offset_{0};
  std::string transaction_id_;
};

ROCKETMQ_NAMESPACE_END