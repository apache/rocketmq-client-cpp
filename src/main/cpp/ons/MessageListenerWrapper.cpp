#include "MessageListenerWrapper.h"
#include "ONSUtil.h"
#include "ons/ConsumeContext.h"
#include "rocketmq/RocketMQ.h"

ONS_NAMESPACE_BEGIN

MessageListenerWrapper::MessageListenerWrapper(ons::MessageListener* message_listener)
    : message_listener_(message_listener) {
}

ROCKETMQ_NAMESPACE::ConsumeMessageResult
MessageListenerWrapper::consumeMessage(const std::vector<ROCKETMQ_NAMESPACE::MQMessageExt>& msgs) {
  ConsumeContext consume_context;
  ONSUtil ons_util = ONSUtil::get();
  ROCKETMQ_NAMESPACE::MQMessageExt mq_message = *msgs.begin();
  Message message = ons_util.msgConvert(mq_message);
  Action action = message_listener_->consume(message, consume_context);

  switch (action) {
    case Action::CommitMessage:
      return ROCKETMQ_NAMESPACE::ConsumeMessageResult::SUCCESS;

    case Action::ReconsumeLater:
    default:
      return ROCKETMQ_NAMESPACE::ConsumeMessageResult::FAILURE;
  }
}

ONS_NAMESPACE_END