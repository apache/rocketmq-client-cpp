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
#ifndef __MQ_MESSAGE_H__
#define __MQ_MESSAGE_H__

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "MQMessageConst.h"

namespace rocketmq {

class MQMessage;
typedef MQMessage* MQMessagePtr;
typedef std::shared_ptr<MQMessage> MQMessagePtr2;
typedef std::unique_ptr<MQMessage> MQMessagePtr3;

class ROCKETMQCLIENT_API MQMessage {
 public:
  MQMessage();
  MQMessage(const std::string& topic, const std::string& body);
  MQMessage(const std::string& topic, const std::string& tags, const std::string& body);
  MQMessage(const std::string& topic, const std::string& tags, const std::string& keys, const std::string& body);
  MQMessage(const std::string& topic,
            const std::string& tags,
            const std::string& keys,
            const int flag,
            const std::string& body,
            bool waitStoreMsgOK);

  MQMessage(const MQMessage& other);
  MQMessage& operator=(const MQMessage& other);

  virtual ~MQMessage();

  const std::string& getProperty(const std::string& name) const;
  void putProperty(const std::string& name, const std::string& value);
  void clearProperty(const std::string& name);

  const std::string& getTopic() const;
  void setTopic(const std::string& topic);
  void setTopic(const char* body, int len);

  const std::string& getTags() const;
  void setTags(const std::string& tags);

  const std::string& getKeys() const;
  void setKeys(const std::string& keys);
  void setKeys(const std::vector<std::string>& keys);

  int getDelayTimeLevel() const;
  void setDelayTimeLevel(int level);

  bool isWaitStoreMsgOK() const;
  void setWaitStoreMsgOK(bool waitStoreMsgOK);

  int getFlag() const;
  void setFlag(int flag);

  const std::string& getBody() const;
  void setBody(const char* body, int len);
  void setBody(const std::string& body);
  void setBody(std::string&& body);

  const std::string& getTransactionId() const;
  void setTransactionId(const std::string& transactionId);

  const std::map<std::string, std::string>& getProperties() const;
  void setProperties(const std::map<std::string, std::string>& properties);
  void setProperties(std::map<std::string, std::string>&& properties);

  virtual bool isBatch() { return false; }

  virtual std::string toString() const {
    std::stringstream ss;
    ss << "Message [topic=" << m_topic << ", flag=" << m_flag << ", tag=" << getTags() << ", transactionId='"
       << m_transactionId + "']";
    return ss.str();
  }

 protected:
  std::string m_topic;
  int m_flag;
  std::map<std::string, std::string> m_properties;
  std::string m_body;
  std::string m_transactionId;
};

}  // namespace rocketmq

#endif  // __MQ_MESSAGE_H__
