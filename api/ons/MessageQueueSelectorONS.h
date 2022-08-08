#pragma once

#include "Message.h"
#include "MessageQueueONS.h"

ONS_NAMESPACE_BEGIN

class MessageQueueSelectorONS {
public:
  virtual ~MessageQueueSelectorONS() = default;

  virtual MessageQueueONS select(const std::vector<MessageQueueONS>& mqs, const Message& msg, void* arg) = 0;
};

ONS_NAMESPACE_END