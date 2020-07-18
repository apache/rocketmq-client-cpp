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
#ifndef ROCKETMQ_MESSAGE_MESSAGEIMPL_H_
#define ROCKETMQ_MESSAGE_MESSAGEIMPL_H_

#include "Message.h"
#include "noncopyable.h"

namespace rocketmq {

/**
 * MessageImpl - Default Message implement
 */
class MessageImpl : public noncopyable,     // base
                    virtual public Message  // interface
{
 public:
  MessageImpl();
  MessageImpl(const std::string& topic, const std::string& body);
  MessageImpl(const std::string& topic,
              const std::string& tags,
              const std::string& keys,
              int32_t flag,
              const std::string& body,
              bool waitStoreMsgOK);

  virtual ~MessageImpl();

  const std::string& getProperty(const std::string& name) const override;
  void putProperty(const std::string& name, const std::string& value) override;
  void clearProperty(const std::string& name) override;

  const std::string& getTopic() const override;
  void setTopic(const std::string& topic) override;
  void setTopic(const char* body, int len) override;

  const std::string& getTags() const override;
  void setTags(const std::string& tags) override;

  const std::string& getKeys() const override;
  void setKeys(const std::string& keys) override;
  void setKeys(const std::vector<std::string>& keys) override;

  int getDelayTimeLevel() const override;
  void setDelayTimeLevel(int level) override;

  bool isWaitStoreMsgOK() const override;
  void setWaitStoreMsgOK(bool waitStoreMsgOK) override;

  int32_t getFlag() const override;
  void setFlag(int32_t flag) override;

  const std::string& getBody() const override;
  void setBody(const char* body, int len) override;
  void setBody(const std::string& body) override;
  void setBody(std::string&& body) override;

  const std::string& getTransactionId() const override;
  void setTransactionId(const std::string& transactionId) override;

  const std::map<std::string, std::string>& getProperties() const override;
  void setProperties(const std::map<std::string, std::string>& properties) override;
  void setProperties(std::map<std::string, std::string>&& properties) override;

  std::string toString() const override;

 protected:
  std::string topic_;
  int32_t flag_;
  std::map<std::string, std::string> properties_;
  std::string body_;
  std::string transaction_id_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MESSAGE_MESSAGEIMPL_H_
