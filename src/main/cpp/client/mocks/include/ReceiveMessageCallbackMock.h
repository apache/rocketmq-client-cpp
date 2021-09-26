#pragma once

#include "gmock/gmock.h"

#include "ReceiveMessageCallback.h"

ROCKETMQ_NAMESPACE_BEGIN

class ReceiveMessageCallbackMock : public ReceiveMessageCallback {
public:
  MOCK_METHOD(void, onSuccess, (ReceiveMessageResult &), (override));

  MOCK_METHOD(void, onFailure, (const std::error_code &), (override));
};

ROCKETMQ_NAMESPACE_END