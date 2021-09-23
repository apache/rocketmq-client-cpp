#pragma once

#include <string>

#include "rocketmq/MQMessage.h"
#include "rocketmq/MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN
class SendMessageContext {
public:
  const std::string& getProducerGroup() const { return producer_group_; };
  void setProducerGroup(const std::string& producer_group) { this->producer_group_ = producer_group; };
  const MQMessage& getMessage() const { return message_; }
  void setMessage(const MQMessage& message) { this->message_ = message; }
  const MQMessageQueue& getMessageQueue() const { return message_queue_; }
  void setMessageQueue(const MQMessageQueue& message_queue) { this->message_queue_ = message_queue; }
  const std::string& getBornHost() const { return born_host_; }
  void setBornHost(const std::string& born_host) { this->born_host_ = born_host; }
  const std::string& getMessageId() const { return message_id_; }
  void setMessageId(const std::string& message_id) { this->message_id_ = message_id; }
  long long int getQueueOffset() const { return queue_offset_; }
  void setQueueOffset(long long queue_offset) { this->queue_offset_ = queue_offset; }

private:
  std::string producer_group_;
  MQMessage message_;
  MQMessageQueue message_queue_;
  std::string born_host_;
  std::string message_id_;
  long long queue_offset_{-1};
};

ROCKETMQ_NAMESPACE_END