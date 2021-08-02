#pragma once
#include "ReceiveMessageCallback.h"
#include "gmock/gmock.h"

ROCKETMQ_NAMESPACE_BEGIN

class ReceiveMessageCallbackMock : public ReceiveMessageCallback {
public:
  MOCK_METHOD(void, onSuccess, (ReceiveMessageResult&), (override));

  MOCK_METHOD(void, onException, (MQException&), (override));
};

ROCKETMQ_NAMESPACE_END