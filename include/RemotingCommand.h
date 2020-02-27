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
#ifndef __REMOTING_COMMAND_H__
#define __REMOTING_COMMAND_H__

#include <map>
#include <memory>
#include <typeindex>

#include "CommandCustomHeader.h"
#include "MQClientException.h"

namespace rocketmq {

class MemoryBlock;
typedef MemoryBlock* MemoryBlockPtr;
typedef std::shared_ptr<MemoryBlock> MemoryBlockPtr2;

class ROCKETMQCLIENT_API RemotingCommand {
 public:
  static int32_t createNewRequestId();

 public:
  RemotingCommand() : m_code(0) {}
  RemotingCommand(int32_t code, CommandCustomHeader* customHeader = nullptr);
  RemotingCommand(int32_t code,
                  const std::string& language,
                  int32_t version,
                  int32_t opaque,
                  int32_t flag,
                  const std::string& remark,
                  CommandCustomHeader* customHeader);
  RemotingCommand(RemotingCommand&& command);

  virtual ~RemotingCommand();

  int32_t getCode() const;
  void setCode(int32_t code);

  int32_t getVersion() const;

  int32_t getOpaque() const;
  void setOpaque(int32_t opaque);

  int32_t getFlag() const;

  const std::string& getRemark() const;
  void setRemark(const std::string& mark);

  bool isResponseType();
  void markResponseType();

  bool isOnewayRPC();
  void markOnewayRPC();

  void addExtField(const std::string& key, const std::string& value);

  CommandCustomHeader* readCustomHeader() const;

  MemoryBlockPtr2 getBody();
  void setBody(MemoryBlock* body);
  void setBody(MemoryBlockPtr2 body);
  void setBody(const std::string& body);

 public:
  MemoryBlockPtr encode();

  template <class H>
  H* decodeCommandCustomHeader(bool useCache);

  template <class H>
  H* decodeCommandCustomHeader();

  static RemotingCommand* Decode(MemoryBlockPtr2& package);

  std::string toString() const;

 private:
  int32_t m_code;
  std::string m_language;
  int32_t m_version;
  int32_t m_opaque;
  int32_t m_flag;
  std::string m_remark;
  std::map<std::string, std::string> m_extFields;
  std::unique_ptr<CommandCustomHeader> m_customHeader;  // transient

  MemoryBlockPtr2 m_body;  // transient
};

template <class H>
H* RemotingCommand::decodeCommandCustomHeader(bool useCache) {
  auto* cache = m_customHeader.get();
  if (cache != nullptr && useCache && std::type_index(typeid(*cache)) == std::type_index(typeid(H))) {
    return static_cast<H*>(m_customHeader.get());
  }
  return decodeCommandCustomHeader<H>();
}

template <class H>
H* RemotingCommand::decodeCommandCustomHeader() {
  try {
    H* header = H::Decode(m_extFields);
    m_customHeader.reset(header);
    return header;
  } catch (std::exception& e) {
    THROW_MQEXCEPTION(RemotingCommandException, e.what(), -1);
  }
}

}  // namespace rocketmq

#endif  // __REMOTING_COMMAND_H__
