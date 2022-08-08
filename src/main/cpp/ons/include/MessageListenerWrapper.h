#pragma once

#include "ons/MessageListener.h"
#include "rocketmq/MessageListener.h"
#include "rocketmq/RocketMQ.h"

ONS_NAMESPACE_BEGIN

class MessageListenerWrapper : public ROCKETMQ_NAMESPACE::StandardMessageListener {

public:
  explicit MessageListenerWrapper(ons::MessageListener* message_listener);

  ROCKETMQ_NAMESPACE::ConsumeMessageResult
  consumeMessage(const std::vector<ROCKETMQ_NAMESPACE::MQMessageExt>& msgs) override;

private:
  ons::MessageListener* message_listener_{nullptr};
};

ONS_NAMESPACE_END