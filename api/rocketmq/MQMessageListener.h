#pragma once

#include "MQMessageExt.h"
#include "MQMessageQueue.h"
#include <limits.h>
#include "iostream"

ROCKETMQ_NAMESPACE_BEGIN

enum ConsumeStatus { CONSUME_SUCCESS, RECONSUME_LATER };

enum MessageListenerType { messageListenerDefaultly = 0, messageListenerOrderly = 1, messageListenerConcurrently = 2 };

class MQMessageListener {
public:
  virtual ~MQMessageListener() {}
  virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt>& msgs) = 0;
  virtual MessageListenerType getMessageListenerType() { return messageListenerDefaultly; }
};

class MessageListenerOrderly : public MQMessageListener {
public:
  virtual ~MessageListenerOrderly() {}
  virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt>& msgs) = 0;
  virtual MessageListenerType getMessageListenerType() { return messageListenerOrderly; }
};

class MessageListenerConcurrently : public MQMessageListener {
public:
  virtual ~MessageListenerConcurrently() {}
  virtual ConsumeStatus consumeMessage(const std::vector<MQMessageExt>& msgs) = 0;
  virtual MessageListenerType getMessageListenerType() { return messageListenerConcurrently; }
};

ROCKETMQ_NAMESPACE_END