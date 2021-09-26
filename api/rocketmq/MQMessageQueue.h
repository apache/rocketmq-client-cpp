#pragma once

#include <functional>
#include <sstream>
#include <string>

#include "RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class MQMessageQueue {
public:
  MQMessageQueue() = default;

  MQMessageQueue(std::string topic, std::string broker_name, int queue_id)
      : topic_(std::move(topic)), broker_name_(std::move(broker_name)),
        queue_id_(queue_id) {}

  MQMessageQueue(const MQMessageQueue &other) { this->operator=(other); }

  MQMessageQueue(MQMessageQueue &&other) noexcept {
    topic_ = std::move(other.topic_);
    broker_name_ = std::move(other.broker_name_);
    queue_id_ = other.queue_id_;
    service_address_ = other.service_address_;
  }

  MQMessageQueue &operator=(const MQMessageQueue &other) {
    if (this == &other) {
      return *this;
    }

    topic_ = other.topic_;
    broker_name_ = other.broker_name_;
    queue_id_ = other.queue_id_;
    service_address_ = other.service_address_;
    return *this;
  }

  const std::string &getTopic() const { return topic_; }

  void setTopic(const std::string &topic) { topic_ = topic; }

  const std::string &getBrokerName() const { return broker_name_; }

  void setBrokerName(const std::string &broker_name) {
    broker_name_ = broker_name;
  }

  int getQueueId() const { return queue_id_; }

  void setQueueId(int queue_id) { queue_id_ = queue_id; }

  bool operator!=(const MQMessageQueue &rhs) const {
    return topic_ != rhs.topic_ || broker_name_ != rhs.broker_name_ ||
           queue_id_ != rhs.queue_id_;
  }

  bool operator==(const MQMessageQueue &mq) const {
    return topic_ == mq.topic_ && broker_name_ == mq.broker_name_ &&
           queue_id_ == mq.queue_id_;
  }

  bool operator<(const MQMessageQueue &mq) const {
    if (topic_ != mq.topic_) {
      return topic_ < mq.topic_;
    }

    if (broker_name_ != mq.broker_name_) {
      return broker_name_ < mq.broker_name_;
    }

    return queue_id_ < mq.queue_id_;
  }

  operator bool() const {
    return !topic_.empty() && !broker_name_.empty() && queue_id_ >= 0;
  }

  int compareTo(const MQMessageQueue &mq) const {
    if (this == &mq) {
      return 0;
    }

    if (this->operator<(mq)) {
      return -1;
    }

    if (mq.operator<(*this)) {
      return 1;
    }

    return 0;
  }

  std::string simpleName() const {
    return topic_ + "_" + broker_name_ + "_" + std::to_string(queue_id_);
  }

  std::string toString() const {
    std::stringstream ss;
    ss << "MessageQueue [topic=" << topic_ << ", brokerName=" << broker_name_
       << ", queueId=" << queue_id_ << "]";
    return ss.str();
  }

  template <typename H> friend H AbslHashValue(H h, const MQMessageQueue &mq) {
    return H::combine(std::move(h), mq.topic_, mq.broker_name_, mq.queue_id_);
  }

  const std::string &serviceAddress() const { return service_address_; }

  void serviceAddress(std::string service_address) {
    service_address_ = std::move(service_address);
  }

private:
  std::string topic_;
  std::string broker_name_;
  int queue_id_{0};

  std::string service_address_;
};

ROCKETMQ_NAMESPACE_END