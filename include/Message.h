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
#ifndef ROCKETMQ_MESSAGE_H_
#define ROCKETMQ_MESSAGE_H_

#include <map>     // std::map
#include <string>  // std::string
#include <vector>  // std::vector
#include <memory>  // std::shared_ptr

#include "RocketMQClient.h"

namespace rocketmq {

class Message;
typedef std::shared_ptr<Message> MessagePtr;

/**
 * Message - interface for messgae
 */
class ROCKETMQCLIENT_API Message {
 public:
  virtual ~Message() = default;

 public:
  // topic
  virtual const std::string& topic() const = 0;
  virtual void set_topic(const std::string& topic) = 0;
  virtual void set_topic(const char* topic, int len) = 0;

  // tags
  virtual const std::string& tags() const = 0;
  virtual void set_tags(const std::string& tags) = 0;

  // keys
  virtual const std::string& keys() const = 0;
  virtual void set_keys(const std::string& keys) = 0;
  virtual void set_keys(const std::vector<std::string>& keys) = 0;

  // delay time level
  virtual int delay_time_level() const = 0;
  virtual void set_delay_time_level(int level) = 0;

  // wait store message ok
  virtual bool wait_store_msg_ok() const = 0;
  virtual void set_wait_store_msg_ok(bool waitStoreMsgOK) = 0;

  // flag
  virtual int32_t flag() const = 0;
  virtual void set_flag(int32_t flag) = 0;

  // body
  virtual const std::string& body() const = 0;
  virtual void set_body(const std::string& body) = 0;
  virtual void set_body(std::string&& body) = 0;

  // transaction id
  virtual const std::string& transaction_id() const = 0;
  virtual void set_transaction_id(const std::string& transactionId) = 0;

  // properties
  virtual const std::map<std::string, std::string>& properties() const = 0;
  virtual void set_properties(const std::map<std::string, std::string>& properties) = 0;
  virtual void set_properties(std::map<std::string, std::string>&& properties) = 0;

 public:
  // property
  virtual const std::string& getProperty(const std::string& name) const = 0;
  virtual void putProperty(const std::string& name, const std::string& value) = 0;
  virtual void clearProperty(const std::string& name) = 0;

  // batch flag
  virtual bool isBatch() const { return false; }

  virtual std::string toString() const = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MESSAGE_H_
