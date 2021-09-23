#pragma once

#include "grpcpp/impl/codegen/interceptor.h"

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class InterceptorContinuation {
public:
  explicit InterceptorContinuation(grpc::experimental::InterceptorBatchMethods* methods) : methods_(methods) {}

  ~InterceptorContinuation() {
    if (!hijacked_) {
      methods_->Proceed();
    }
  }

  void hijack() { hijacked_ = true; }

private:
  grpc::experimental::InterceptorBatchMethods* methods_;
  bool hijacked_{false};
};

ROCKETMQ_NAMESPACE_END