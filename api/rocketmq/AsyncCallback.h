#pragma once

#include "MQClientException.h"
#include "PullResult.h"
#include "SendResult.h"

ROCKETMQ_NAMESPACE_BEGIN

struct AsyncCallback {};

enum class SendCallbackType : int8_t { noAutoDeleteSendCallback = 0, autoDeleteSendCallback = 1 };

using sendCallbackType = SendCallbackType;

class SendCallback : public AsyncCallback {
public:
  virtual ~SendCallback() = default;

  virtual void onSuccess(SendResult& send_result) = 0;

  virtual void onException(const MQException& e) = 0;

  virtual SendCallbackType getSendCallbackType() { return SendCallbackType::noAutoDeleteSendCallback; }
};

class PullCallback : public AsyncCallback {
public:
  virtual ~PullCallback() = default;

  virtual void onSuccess(const PullResult& pull_result) = 0;

  virtual void onException(const MQException& e) = 0;
};

ROCKETMQ_NAMESPACE_END