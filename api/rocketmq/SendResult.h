#pragma once

#include <utility>

#include "MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class SendStatus : int8_t { SEND_OK, SEND_FLUSH_DISK_TIMEOUT, SEND_FLUSH_SLAVE_TIMEOUT, SEND_SLAVE_NOT_AVAILABLE };

class SendResult {
 public:
  SendResult() {}

  SendResult(const SendStatus& send_status, std::string  msg_id, MQMessageQueue  message_queue,
             long long queue_offset, std::string  region_id)
      : send_status_(send_status),
        message_id_(std::move(msg_id)),
        message_queue_(std::move(message_queue)),
        queue_offset_(queue_offset),
        region_id_(std::move(region_id)) {}

  ~SendResult() {}

  SendResult(const SendResult& other) { this->operator=(other); }

  SendResult& operator=(const SendResult& other) {
    if (this == &other) {
      return *this;
    }

    send_status_ = other.send_status_;
    message_id_ = other.message_id_;
    message_queue_ = other.message_queue_;
    queue_offset_ = other.queue_offset_;
    transaction_id_ = other.transaction_id_;
    region_id_ = other.region_id_;
    return *this;
  }

  const std::string& getMsgId() const { return message_id_; }

  void setMsgId(const std::string& msg_id) { message_id_ = msg_id; }

  const std::string& getRegionId() const { return region_id_; }

  void setRegionId(const std::string& region_id) { region_id_ = region_id; }

  SendStatus getSendStatus() const { return send_status_; }

  void setSendStatus(const SendStatus& send_status) { send_status_ = send_status; }

  MQMessageQueue& getMessageQueue() const { return message_queue_; }

  void setMessageQueue(const MQMessageQueue& message_queue) { message_queue_ = message_queue; }

  long long getQueueOffset() const { return queue_offset_; }

  void setQueueOffset(long long queue_offset) { queue_offset_ = queue_offset; }

  const std::string& getTransactionId() const { return transaction_id_; }

  void setTransactionId(const std::string& transaction_id) { transaction_id_ = transaction_id; }

 private:
  SendStatus send_status_;
  std::string message_id_;
  mutable MQMessageQueue message_queue_;
  long long queue_offset_;
  std::string transaction_id_;
  std::string region_id_;
};

ROCKETMQ_NAMESPACE_END