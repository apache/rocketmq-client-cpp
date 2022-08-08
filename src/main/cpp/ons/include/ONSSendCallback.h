#pragma once

#include <cassert>
#include <system_error>

#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"

#include "ons/ONSCallback.h"
#include "rocketmq/AsyncCallback.h"
#include "rocketmq/MQClientException.h"
#include "rocketmq/SendResult.h"

ONS_NAMESPACE_BEGIN

class ONSSendCallback : public ROCKETMQ_NAMESPACE::SendCallback {

public:
  ONSSendCallback() = default;

  static ONSSendCallback* instance() {
    if (instance_ == nullptr) {
      absl::MutexLock lock(&mutex_);
      if (instance_ == nullptr) {
        instance_ = new ONSSendCallback();
      }
    }

    return instance_;
  }

  ~ONSSendCallback() override = default;

  void onSuccess(ROCKETMQ_NAMESPACE::SendResult& send_result) noexcept override {
    SendResultONS resultONS;
    resultONS.setMessageId(send_result.getMsgId());
    assert(callback_ != nullptr);
    callback_->onSuccess(resultONS);
  }

  void onFailure(const std::error_code& ec) noexcept override {
    ONSClientException ons_exception(ec.message(), ec.value());
    assert(callback_ != nullptr);
    callback_->onException(ons_exception);
  }

  void setOnsCallBack(SendCallbackONS* send_callback) {
    callback_ = send_callback;
  }

private:
  SendCallbackONS* callback_{nullptr};

  static absl::Mutex mutex_;

  static ONSSendCallback* instance_;
};

ONS_NAMESPACE_END