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
#ifndef ROCKETMQ_MQMESSAGE_H_
#define ROCKETMQ_MQMESSAGE_H_

#include <algorithm>  // std::move

#include "Message.h"
#include "MQMessageConst.h"

namespace rocketmq {

/**
 * MQMessage - wrapper for Message
 */
class ROCKETMQCLIENT_API MQMessage : virtual public Message  // interface
{
 public:
  MQMessage();
  MQMessage(const std::string& topic, const std::string& body);
  MQMessage(const std::string& topic, const std::string& tags, const std::string& body);
  MQMessage(const std::string& topic, const std::string& tags, const std::string& keys, const std::string& body);
  MQMessage(const std::string& topic,
            const std::string& tags,
            const std::string& keys,
            int32_t flag,
            const std::string& body,
            bool waitStoreMsgOK);

 public:
  // reference constructor
  MQMessage(MessagePtr impl) : message_impl_(impl) {}

  // copy constructor
  MQMessage(const MQMessage& other) : message_impl_(other.message_impl_) {}
  MQMessage(MQMessage&& other) : message_impl_(std::move(other.message_impl_)) {}

  // assign operator
  MQMessage& operator=(const MQMessage& other) {
    if (this != &other) {
      message_impl_ = other.message_impl_;
    }
    return *this;
  }

  bool operator==(std::nullptr_t) const noexcept { return nullptr == message_impl_; }
  friend bool operator==(std::nullptr_t, const MQMessage& message) noexcept;

  // convert to boolean
  operator bool() noexcept { return nullptr != message_impl_; }

 public:
  virtual ~MQMessage();

 public:  // Message
  const std::string& getProperty(const std::string& name) const override;
  void putProperty(const std::string& name, const std::string& value) override;
  void clearProperty(const std::string& name) override;

  const std::string& topic() const override;
  void set_topic(const std::string& topic) override;
  void set_topic(const char* body, int len) override;

  const std::string& tags() const override;
  void set_tags(const std::string& tags) override;

  const std::string& keys() const override;
  void set_keys(const std::string& keys) override;
  void set_keys(const std::vector<std::string>& keys) override;

  int delay_time_level() const override;
  void set_delay_time_level(int level) override;

  bool wait_store_msg_ok() const override;
  void set_wait_store_msg_ok(bool waitStoreMsgOK) override;

  int32_t flag() const override;
  void set_flag(int32_t flag) override;

  const std::string& body() const override;
  void set_body(const std::string& body) override;
  void set_body(std::string&& body) override;

  const std::string& transaction_id() const override;
  void set_transaction_id(const std::string& transactionId) override;

  const std::map<std::string, std::string>& properties() const override;
  void set_properties(const std::map<std::string, std::string>& properties) override;
  void set_properties(std::map<std::string, std::string>&& properties) override;

  bool isBatch() const override;

  std::string toString() const override;

 public:
  MessagePtr getMessageImpl();

 protected:
  MessagePtr message_impl_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQMESSAGE_H_
