#pragma once

#include <cstdlib>
#include <vector>

#include "MQMessageExt.h"

ROCKETMQ_NAMESPACE_BEGIN

class PullResult {
public:
  PullResult(int64_t min, int64_t max, int64_t next, std::vector<MQMessageExt> messages)
      : min_(min), max_(max), next_(next), messages_(std::move(messages)) {}

  int64_t min() const { return min_; }

  int64_t max() const { return max_; }

  int64_t next() const { return next_; }

  const std::vector<MQMessageExt>& messages() const { return messages_; }

private:
  int64_t min_;
  int64_t max_;
  int64_t next_;
  std::vector<MQMessageExt> messages_;
};

ROCKETMQ_NAMESPACE_END