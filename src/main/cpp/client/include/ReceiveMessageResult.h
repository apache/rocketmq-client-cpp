#pragma once

#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <utility>

#include "MixAll.h"
#include "absl/time/time.h"
#include "rocketmq/MQMessageExt.h"

ROCKETMQ_NAMESPACE_BEGIN

struct ReceiveMessageResult {
  absl::Time pop_time;
  absl::Duration invisible_time;

  std::vector<MQMessageExt> messages;

  std::string source_host;

  std::int64_t min_offset{0};
  std::int64_t next_offset{0};
  std::int64_t max_offset{0};
};

ROCKETMQ_NAMESPACE_END