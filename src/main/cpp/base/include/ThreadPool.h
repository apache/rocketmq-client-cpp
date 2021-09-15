#pragma once

#include "rocketmq/RocketMQ.h"
#include <functional>

ROCKETMQ_NAMESPACE_BEGIN

class ThreadPool {
public:
  virtual ~ThreadPool() = default;

  virtual void start() = 0;

  virtual void shutdown() = 0;

  virtual void submit(std::function<void(void)> task) = 0;
};

ROCKETMQ_NAMESPACE_END