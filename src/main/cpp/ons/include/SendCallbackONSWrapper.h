#pragma once

#include <system_error>

#include "ons/ONSCallback.h"
#include "rocketmq/AsyncCallback.h"
#include "rocketmq/RocketMQ.h"

ONS_NAMESPACE_BEGIN

class SendCallbackONSWrapper : public ROCKETMQ_NAMESPACE::SendCallback {
public:
  explicit SendCallbackONSWrapper(SendCallbackONS* callback);

  void onSuccess(ROCKETMQ_NAMESPACE::SendResult& send_result) noexcept override;

  void onFailure(const std::error_code& ec) noexcept override;

private:
  SendCallbackONS* send_callback_ons_ptr_{};
};

ONS_NAMESPACE_END