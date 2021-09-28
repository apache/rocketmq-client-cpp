#pragma once

#include <system_error>

#include "ReceiveMessageResult.h"
#include "rocketmq/AsyncCallback.h"
#include "rocketmq/ErrorCode.h"

ROCKETMQ_NAMESPACE_BEGIN

class ReceiveMessageCallback : public AsyncCallback {
public:
  ~ReceiveMessageCallback() override = default;

  virtual void onCompletion(const std::error_code& ec, const ReceiveMessageResult& result) = 0;
};

ROCKETMQ_NAMESPACE_END