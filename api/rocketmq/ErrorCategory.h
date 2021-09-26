#pragma once

#include "ErrorCode.h"

ROCKETMQ_NAMESPACE_BEGIN

class ErrorCategory : public std::error_category {
public:
  static const ErrorCategory& instance() {
    static ErrorCategory instance;
    return instance;
  }

  const char* name() const noexcept override { return "RocketMQ"; }

  std::string message(int code) const override;

private:
  ErrorCategory() = default;
};

ROCKETMQ_NAMESPACE_END