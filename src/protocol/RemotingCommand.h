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
#ifndef __REMOTINGCOMMAND_H__
#define __REMOTINGCOMMAND_H__

#include <atomic>
#include <memory>
#include <sstream>
#include "CommandHeader.h"
#include "dataBlock.h"

namespace rocketmq {

const int RPC_TYPE = 0;    // 0, REQUEST_COMMAND // 1, RESPONSE_COMMAND;
const int RPC_ONEWAY = 1;  // 0, RPC // 1, Oneway;

class RemotingCommand {
 public:
  RemotingCommand() : m_code(0){};
  RemotingCommand(int code, CommandHeader* pCustomHeader = NULL);
  RemotingCommand(int code,
                  std::string language,
                  int version,
                  int opaque,
                  int flag,
                  std::string remark,
                  CommandHeader* pCustomHeader);
  RemotingCommand(const RemotingCommand& command);
  RemotingCommand& operator=(const RemotingCommand& command);
  virtual ~RemotingCommand();
  const MemoryBlock* GetHead() const;
  const MemoryBlock* GetBody() const;
  void SetBody(const char* pData, int len);
  void setOpaque(const int opa);
  void SetExtHeader(int code);
  void setCode(int code);
  int getCode() const;
  int getOpaque() const;
  void setRemark(std::string mark);
  std::string getRemark() const;
  void markResponseType();
  bool isResponseType();
  void markOnewayRPC();
  bool isOnewayRPC();
  void setParsedJson(Json::Value json);
  CommandHeader* getCommandHeader() const;
  const int getFlag() const;
  const int getVersion() const;
  void addExtField(const std::string& key, const std::string& value);
  std::string getMsgBody() const;
  void setMsgBody(const std::string& body);

 public:
  void Encode();
  static RemotingCommand* Decode(const MemoryBlock& mem);
  std::string ToString() const;

 private:
  void Assign(const RemotingCommand& command);

 private:
  int m_code;
  std::string m_language;
  int m_version;
  int m_opaque;
  int m_flag;
  std::string m_remark;
  std::string m_msgBody;
  std::map<std::string, std::string> m_extFields;

  MemoryBlock m_head;
  MemoryBlock m_body;

  //<!save here
  Json::Value m_parsedJson;
  std::unique_ptr<CommandHeader> m_pExtHeader;

  static std::atomic<int> s_seqNumber;
};

}  // namespace rocketmq

#endif
