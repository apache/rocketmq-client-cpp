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
#pragma once

#include <chrono>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/strings/string_view.h"

#include "MQMessageQueue.h"
#include "MessageType.h"

ROCKETMQ_NAMESPACE_BEGIN

class MessageImpl;

class MessageAccessor;

class MQMessage {
public:
  MQMessage();
  MQMessage(const std::string& topic, const std::string& body);
  MQMessage(const std::string& topic, const std::string& tags, const std::string& body);
  MQMessage(const std::string& topic, const std::string& tags, const std::string& keys, const std::string& body);

  virtual ~MQMessage();

  MQMessage(const MQMessage& other);
  MQMessage& operator=(const MQMessage& other);

  const std::string& getMsgId() const;

  void setProperty(const std::string& name, const std::string& value);
  std::string getProperty(const std::string& name) const;

  const std::string& getTopic() const;
  void setTopic(const std::string& topic);
  void setTopic(const char* data, int len);

  std::string getTags() const;
  void setTags(const std::string& tags);

  const std::vector<std::string>& getKeys() const;

  /**
   * @brief Add a unique key for the message
   * TODO: a message may be associated with multiple keys. setKey, actually mean
   * attach the given key to the message. Better rename it.
   * @param key Unique key in perspective of bussiness logic.
   */
  void setKey(const std::string& key);
  void setKeys(const std::vector<std::string>& keys);

  int getDelayTimeLevel() const;
  void setDelayTimeLevel(int level);

  const std::string& traceContext() const;
  void traceContext(const std::string& trace_context);

  std::string getBornHost() const;

  std::chrono::system_clock::time_point deliveryTimestamp() const;

  const std::string& getBody() const;
  void setBody(const char* data, int len);
  void setBody(const std::string& body);

  uint32_t bodyLength() const;

  const std::map<std::string, std::string>& getProperties() const;
  void setProperties(const std::map<std::string, std::string>& properties);

  void messageType(MessageType message_type);
  MessageType messageType() const;

  void bindMessageGroup(absl::string_view message_group);

  void bindMessageQueue(const MQMessageQueue& message_queue) {
    message_queue_ = message_queue;
  }

  const std::string& messageGroup() const;

  const MQMessageQueue& messageQueue() const {
    return message_queue_;
  }

protected:
  MessageImpl* impl_;

  friend class MessageAccessor;

private:
  MQMessageQueue message_queue_;
};

ROCKETMQ_NAMESPACE_END