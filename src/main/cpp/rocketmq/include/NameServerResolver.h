#pragma once

#include <string>
#include <vector>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class NameServerResolver {
public:
  virtual ~NameServerResolver() = default;

  virtual void start() = 0;

  virtual void shutdown() = 0;

  virtual std::string next() = 0;

  virtual std::string current() = 0;

  virtual std::vector<std::string> resolve() = 0;
};

ROCKETMQ_NAMESPACE_END