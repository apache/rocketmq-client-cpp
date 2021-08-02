#pragma once

#include <functional>
#include <cstdlib>
#include <chrono>

#include "MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

enum ConsumeType {
  CONSUME_ACTIVELY,
  CONSUME_PASSIVELY,
};

enum ConsumeFromWhere {
  CONSUME_FROM_LAST_OFFSET,
  CONSUME_FROM_LAST_OFFSET_AND_FROM_MIN_WHEN_BOOT_FIRST,
  CONSUME_FROM_MIN_OFFSET,
  CONSUME_FROM_MAX_OFFSET,
  CONSUME_FROM_FIRST_OFFSET,
  CONSUME_FROM_TIMESTAMP,
};

enum ConsumeInitialMode {
  MIN,
  MAX,
};

enum QueryOffsetPolicy : uint8_t {
  BEGINNING = 0,
  END = 1,
  TIME_POINT = 2,
};

struct OffsetQuery {
  MQMessageQueue message_queue;
  QueryOffsetPolicy policy;
  std::chrono::system_clock::time_point time_point;
};

struct PullMessageQuery {
  MQMessageQueue message_queue;
  int64_t offset;
  int32_t batch_size;
  std::chrono::system_clock::duration await_time;
  std::chrono::system_clock::duration timeout;
};

ROCKETMQ_NAMESPACE_END