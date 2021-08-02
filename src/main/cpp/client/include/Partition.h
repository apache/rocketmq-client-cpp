#pragma once

#include "Broker.h"
#include "Topic.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

enum Permission : int8_t { NONE = 0, READ = 1, WRITE = 2, READ_WRITE = 3 };

class Partition {
public:
  Partition(Topic topic, int32_t id, Permission permission, Broker broker)
      : topic_(std::move(topic)), broker_(std::move(broker)), id_(id), permission_(permission) {}

  const Topic& topic() const { return topic_; }

  int32_t id() const { return id_; }

  const Broker& broker() const { return broker_; }

  Permission permission() const { return permission_; }

  MQMessageQueue asMessageQueue() const {
    MQMessageQueue message_queue(topic_.name(), broker_.name(), id_);
    message_queue.serviceAddress(broker_.serviceAddress());
    return message_queue;
  }

  bool operator==(const Partition& other) const {
    return topic_ == other.topic_ && id_ == other.id_ && broker_ == other.broker_ && permission_ == other.permission_;
  }

  bool operator<(const Partition& other) const {
    if (topic_ < other.topic_) {
      return true;
    } else if (other.topic_ < topic_) {
      return false;
    }

    if (broker_ < other.broker_) {
      return true;
    } else if (other.broker_ < broker_) {
      return false;
    }

    return id_ < other.id_;
  }

private:
  Topic topic_;
  Broker broker_;
  int32_t id_;
  Permission permission_;
};

ROCKETMQ_NAMESPACE_END