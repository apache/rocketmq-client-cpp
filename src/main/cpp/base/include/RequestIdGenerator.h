#pragma once

#include "rocketmq/RocketMQ.h"
#include <cstdlib>
#include <string>
#include <atomic>

ROCKETMQ_NAMESPACE_BEGIN

class RequestIdGenerator {
public:
  static std::string nextRequestId();

  static int64_t sequenceOf(const std::string& request_id);

private:
  static std::atomic_long sequence_;
};

ROCKETMQ_NAMESPACE_END