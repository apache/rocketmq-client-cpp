#pragma once

#include "rocketmq/MQSelector.h"

ROCKETMQ_NAMESPACE_BEGIN

class MessageGroupQueueSelector : public MessageQueueSelector {
public:
  explicit MessageGroupQueueSelector(std::string message_group);

  MQMessageQueue select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg, void* arg) override;

private:
  std::string message_group_;
};

ROCKETMQ_NAMESPACE_END