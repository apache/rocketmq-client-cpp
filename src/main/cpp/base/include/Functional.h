#pragma once

#include "rocketmq/RocketMQ.h"
#include <functional>
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN

class Functional {
public:
  Functional() = default;

  explicit Functional(std::function<void(void)>* function) : function_(function) {}

  ~Functional() = default;

  void setFunction(std::function<void(void)>* function) { function_ = function; }

  std::function<void()>* getFunction() { return function_; }

private:
  std::function<void(void)>* function_;
};

using FunctionalSharePtr = std::shared_ptr<Functional>;
using FunctionalWeakPtr = std::weak_ptr<Functional>;

ROCKETMQ_NAMESPACE_END