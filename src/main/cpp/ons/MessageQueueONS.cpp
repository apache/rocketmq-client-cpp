#include "ons/MessageQueueONS.h"
#include "ons/ONSClientException.h"

ONS_NAMESPACE_BEGIN

std::string MessageQueueONS::getTopic() const {
  return topic_;
}

void MessageQueueONS::setTopic(const std::string& topic) {
  topic_ = topic;
}

std::string MessageQueueONS::getBrokerName() const {
  return broker_name_;
}

void MessageQueueONS::setBrokerName(const std::string& broker_name) {
  broker_name_ = broker_name;
}

int MessageQueueONS::getQueueId() const {
  return queue_id_;
}

void MessageQueueONS::setQueueId(int queue_id) {
  queue_id_ = queue_id;
}

bool MessageQueueONS::operator==(const MessageQueueONS& mq) const {
  if (this == &mq) {
    return true;
  }

  if (broker_name_ != mq.broker_name_) {
    return false;
  }

  if (queue_id_ != mq.queue_id_) {
    return false;
  }

  if (topic_ != mq.topic_) {
    return false;
  }

  return true;
}

int MessageQueueONS::compareTo(const MessageQueueONS& mq) const {
  int result = topic_.compare(mq.topic_);
  if (result != 0) {
    return result;
  }

  result = broker_name_.compare(mq.broker_name_);
  if (result != 0) {
    return result;
  }

  return queue_id_ - mq.queue_id_;
}

bool MessageQueueONS::operator<(const MessageQueueONS& mq) const {
  return compareTo(mq) < 0;
}

ONS_NAMESPACE_END