#pragma once

#include "rocketmq/RocketMQ.h"
#include <string>
#include <unordered_map>

ROCKETMQ_NAMESPACE_BEGIN

class Topic {
public:
  Topic(std::string arn, std::string name) : arn_(std::move(arn)), name_(std::move(name)) {}

  const std::string& arn() const { return arn_; }

  const std::string& name() const { return name_; }

  bool operator==(const Topic& other) const { return arn_ == other.arn_ && name_ == other.name_; }

  bool operator<(const Topic& other) const {
    if (arn_ < other.arn_) {
      return true;
    } else if (arn_ > other.arn_) {
      return false;
    }

    if (name_ < other.name_) {
      return true;
    } else if (name_ > other.name_) {
      return false;
    }
    return false;
  }

private:
  std::string arn_;
  std::string name_;
};

ROCKETMQ_NAMESPACE_END