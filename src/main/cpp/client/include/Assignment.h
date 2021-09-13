#pragma once

#include "ConsumeMessageType.h"
#include "rocketmq/MQMessageQueue.h"
#include <map>
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

class Assignment {
public:
  Assignment(MQMessageQueue message_queue)
      : message_queue_(std::move(message_queue)) {}

  bool operator==(const Assignment &rhs) const {
    if (this == &rhs) {
      return true;
    }

    return message_queue_ == rhs.message_queue_;
  }

  bool operator<(const Assignment &rhs) const {
    return message_queue_ < rhs.message_queue_;
  }

  const MQMessageQueue &messageQueue() const { return message_queue_; }

private:
  MQMessageQueue message_queue_;
};
ROCKETMQ_NAMESPACE_END