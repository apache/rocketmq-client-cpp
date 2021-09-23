#pragma once

#include <string>
#include <unordered_map>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class Topic {
public:
  Topic(std::string resource_namespace, std::string name)
      : resource_namespace_(std::move(resource_namespace)), name_(std::move(name)) {}

  const std::string& resourceNamespace() const { return resource_namespace_; }

  const std::string& name() const { return name_; }

  bool operator==(const Topic& other) const {
    return resource_namespace_ == other.resource_namespace_ && name_ == other.name_;
  }

  bool operator<(const Topic& other) const {
    if (resource_namespace_ < other.resource_namespace_) {
      return true;
    } else if (resource_namespace_ > other.resource_namespace_) {
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
  std::string resource_namespace_;
  std::string name_;
};

ROCKETMQ_NAMESPACE_END