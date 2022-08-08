#pragma once

#include <cassert>

#include "ONSUtil.h"
#include "ons/ConsumeOrderContext.h"
#include "ons/MessageOrderListener.h"
#include "ons/OrderAction.h"
#include "rocketmq/MessageListener.h"

ONS_NAMESPACE_BEGIN

class OrderListenerWrapper : public ROCKETMQ_NAMESPACE::FifoMessageListener {
public:
  explicit OrderListenerWrapper(MessageOrderListener* listener) : wrapped_listener_(listener) {
    assert(wrapped_listener_);
  }

  ROCKETMQ_NAMESPACE::ConsumeMessageResult consumeMessage(const ROCKETMQ_NAMESPACE::MQMessageExt& msg) override {
    auto&& message = ONSUtil::get().msgConvert(msg);
    ConsumeOrderContext context;
    OrderAction action = wrapped_listener_->consume(message, context);
    switch (action) {
      case OrderAction::Success:
        return ROCKETMQ_NAMESPACE::ConsumeMessageResult::SUCCESS;

      case OrderAction::Suspend:
        return ROCKETMQ_NAMESPACE::ConsumeMessageResult::FAILURE;
      default:
        return ROCKETMQ_NAMESPACE::ConsumeMessageResult::SUCCESS;
    }
  }

private:
  MessageOrderListener* wrapped_listener_;
};

ONS_NAMESPACE_END