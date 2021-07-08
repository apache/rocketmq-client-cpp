#pragma once

#include "ReceiveMessageResult.h"
#include "rocketmq/AsyncCallback.h"

ROCKETMQ_NAMESPACE_BEGIN
class ReceiveMessageCallback : public AsyncCallback {
public:
  virtual ~ReceiveMessageCallback() = default;
  virtual void onSuccess(ReceiveMessageResult& result) = 0;
  virtual void onException(MQException& e) = 0;
};
ROCKETMQ_NAMESPACE_END