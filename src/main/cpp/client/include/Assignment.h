#pragma once

#include "rocketmq/MQMessageQueue.h"
#include <map>
#include <string>

namespace rocketmq {

enum ConsumeMessageType {
  PULL = 0,
  POP = 1,
};

class Assignment {
public:
  Assignment(MQMessageQueue message_queue, ConsumeMessageType consume_type)
      : message_queue_(std::move(message_queue)), consume_type_(consume_type) {}

  bool operator==(const Assignment& rhs) const {
    if (this == &rhs) {
      return true;
    }

    return message_queue_ == rhs.message_queue_ && consume_type_ == rhs.consume_type_;
  }

  bool operator<(const Assignment& rhs) const {
    if (message_queue_ != rhs.message_queue_) {
      return message_queue_ < rhs.message_queue_;
    }

    if (consume_type_ != rhs.consume_type_) {
      return consume_type_ < rhs.consume_type_;
    }

    return false;
  }

  const MQMessageQueue& messageQueue() const { return message_queue_; }

  ConsumeMessageType consumeType() const { return consume_type_; }

private:
  MQMessageQueue message_queue_;
  ConsumeMessageType consume_type_;
};

} // namespace rocketmq