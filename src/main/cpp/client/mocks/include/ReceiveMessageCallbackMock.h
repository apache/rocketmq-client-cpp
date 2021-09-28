#pragma once

#include "gmock/gmock.h"

#include "ReceiveMessageCallback.h"

ROCKETMQ_NAMESPACE_BEGIN

class ReceiveMessageCallbackMock : public ReceiveMessageCallback {
public:
  MOCK_METHOD(void, onCompletion, (const std::error_code&, const ReceiveMessageResult&), (override));
};

ROCKETMQ_NAMESPACE_END