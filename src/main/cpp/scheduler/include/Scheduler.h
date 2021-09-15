#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class Scheduler {
public:
  virtual ~Scheduler() = default;

  virtual void start() = 0;

  virtual void shutdown() = 0;

  virtual std::uint32_t schedule(const std::function<void(void)>& functor, const std::string& task_name,
                                 std::chrono::milliseconds delay, std::chrono::milliseconds interval) = 0;

  virtual void cancel(std::uint32_t task_id) = 0;
};

ROCKETMQ_NAMESPACE_END