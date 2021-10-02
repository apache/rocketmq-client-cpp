#pragma once

#include <string>

#include "absl/strings/string_view.h"

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class QueryRouteRequestHeader : public CommandCustomHeader {
public:
  ~QueryRouteRequestHeader() override = default;

  void topic(absl::string_view topic) {
    topic_ = std::string(topic.data(), topic.length());
  }

  const std::string& topic() const {
    return topic_;
  }

  void encode(google::protobuf::Value& root) const override;

private:
  std::string topic_;
};

ROCKETMQ_NAMESPACE_END