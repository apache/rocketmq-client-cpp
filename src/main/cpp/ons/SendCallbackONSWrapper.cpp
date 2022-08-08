
#include "SendCallbackONSWrapper.h"
#include "rocketmq/RocketMQ.h"

ONS_NAMESPACE_BEGIN

SendCallbackONSWrapper::SendCallbackONSWrapper(SendCallbackONS* send_callback_ons_ptr)
    : send_callback_ons_ptr_(send_callback_ons_ptr) {
}

void SendCallbackONSWrapper::onSuccess(ROCKETMQ_NAMESPACE::SendResult& send_result) noexcept {
  if (nullptr == send_callback_ons_ptr_) {
    return;
  }
  SendResultONS send_result_ons;
  send_result_ons.setMessageId(send_result.getMsgId());
  send_callback_ons_ptr_->onSuccess(send_result_ons);
}

void SendCallbackONSWrapper::onFailure(const std::error_code& ec) noexcept {
  if (nullptr == send_callback_ons_ptr_) {
    return;
  }

  ONSClientException ons_client_exception(ec.message(), ec.value());
  send_callback_ons_ptr_->onException(ons_client_exception);
}

ONS_NAMESPACE_END