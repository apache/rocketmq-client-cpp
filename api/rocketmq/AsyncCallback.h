#pragma once

#include <system_error>

#include "MQClientException.h"
#include "PullResult.h"
#include "SendResult.h"

ROCKETMQ_NAMESPACE_BEGIN

class AsyncCallback {
public:
  virtual ~AsyncCallback() = default;
};

class SendCallback : public AsyncCallback {
public:
  ~SendCallback() override = default;

  virtual void onSuccess(SendResult &send_result) noexcept = 0;

  virtual void onFailure(const std::error_code &ec) noexcept = 0;
};

class PullCallback : public AsyncCallback {
public:
  ~PullCallback() override = default;

  virtual void onSuccess(const PullResult &pull_result) noexcept = 0;

  virtual void onFailure(const std::error_code &ec) noexcept = 0;
};

ROCKETMQ_NAMESPACE_END