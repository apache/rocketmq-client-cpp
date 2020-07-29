/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ROCKETMQ_SENDRESULT_H_
#define ROCKETMQ_SENDRESULT_H_

#include "MQMessageQueue.h"

namespace rocketmq {

// all to Master;
enum SendStatus { SEND_OK, SEND_FLUSH_DISK_TIMEOUT, SEND_FLUSH_SLAVE_TIMEOUT, SEND_SLAVE_NOT_AVAILABLE };

class ROCKETMQCLIENT_API SendResult {
 public:
  SendResult() : send_status_(SEND_OK), queue_offset_(0) {}

  SendResult(const SendStatus& sendStatus,
             const std::string& msgId,
             const std::string& offsetMsgId,
             const MQMessageQueue& messageQueue,
             int64_t queueOffset)
      : send_status_(sendStatus),
        msg_id_(msgId),
        offset_msg_id_(offsetMsgId),
        message_queue_(messageQueue),
        queue_offset_(queueOffset) {}

  SendResult(const SendResult& other) {
    send_status_ = other.send_status_;
    msg_id_ = other.msg_id_;
    offset_msg_id_ = other.offset_msg_id_;
    message_queue_ = other.message_queue_;
    queue_offset_ = other.queue_offset_;
  }

  SendResult& operator=(const SendResult& other) {
    if (this != &other) {
      send_status_ = other.send_status_;
      msg_id_ = other.msg_id_;
      offset_msg_id_ = other.offset_msg_id_;
      message_queue_ = other.message_queue_;
      queue_offset_ = other.queue_offset_;
    }
    return *this;
  }

  virtual ~SendResult() = default;

  inline SendStatus send_status() const { return send_status_; }
  inline void send_status(SendStatus send_status) { send_status_ = send_status; }

  inline const std::string& msg_id() const { return msg_id_; }
  inline void msg_id(const std::string& msg_id) { msg_id_ = msg_id; }

  inline const std::string& offset_msg_id() const { return offset_msg_id_; }
  inline void offset_msg_id(std::string& offset_msg_id) { offset_msg_id_ = offset_msg_id; }

  inline const MQMessageQueue& message_queue() const { return message_queue_; }
  inline void message_queue(const MQMessageQueue& message_queue) { message_queue_ = message_queue; }

  inline int64_t queue_offset() const { return queue_offset_; }
  inline void set_queue_offset(int64_t queue_offset) { queue_offset_ = queue_offset; }

  inline const std::string& transaction_id() const { return transaction_id_; }
  inline void set_transaction_id(const std::string& id) { transaction_id_ = id; }

  std::string toString() const;

 private:
  SendStatus send_status_;
  std::string msg_id_;
  std::string offset_msg_id_;
  MQMessageQueue message_queue_;
  int64_t queue_offset_;
  std::string transaction_id_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_SENDRESULT_H_
