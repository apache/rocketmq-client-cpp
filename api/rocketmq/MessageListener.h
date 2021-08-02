#pragma once

#include "MQMessageExt.h"
#include "MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class ConsumeMessageResult : uint8_t { SUCCESS = 0, FAILURE = 1 };

enum class MessageListenerType : uint8_t { STANDARD = 0, FIFO = 1 };

class MessageListener {
public:
  virtual ~MessageListener() = default;

  virtual MessageListenerType listenerType() = 0;
};

class StandardMessageListener : public MessageListener {
public:
  virtual ConsumeMessageResult consumeMessage(const std::vector<MQMessageExt>& msgs) = 0;

  MessageListenerType listenerType() override { return MessageListenerType::STANDARD; }
};

class FifoMessageListener : public MessageListener {
public:
  MessageListenerType listenerType() override { return MessageListenerType::FIFO; }

  virtual ConsumeMessageResult consumeMessage(const MQMessageExt& msgs) = 0;
};

ROCKETMQ_NAMESPACE_END