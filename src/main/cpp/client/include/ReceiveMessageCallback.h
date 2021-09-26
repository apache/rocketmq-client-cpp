#pragma once

#include "ReceiveMessageResult.h"
#include "rocketmq/AsyncCallback.h"
#include "rocketmq/ErrorCode.h"
#include <system_error>

ROCKETMQ_NAMESPACE_BEGIN

class ReceiveMessageCallback : public AsyncCallback {
public:
  ~ReceiveMessageCallback() override = default;

  virtual void onSuccess(ReceiveMessageResult &result) = 0;

  virtual void onFailure(const std::error_code &ec) = 0;
};

ROCKETMQ_NAMESPACE_END