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
#include "RemotingCommand.h"

#include <atomic>
#include <limits>

#include "ByteOrder.h"
#include "Logging.h"
#include "MQVersion.h"
#include "RemotingSerializable.h"

namespace rocketmq {

const int RPC_TYPE = 0;    // 0 - REQUEST_COMMAND; 1 - RESPONSE_COMMAND;
const int RPC_ONEWAY = 1;  // 0 - RPC; 1 - Oneway;

int32_t RemotingCommand::createNewRequestId() {
  static std::atomic<int32_t> sSeqNumber;
  // mask sign bit
  return sSeqNumber.fetch_add(1, std::memory_order_relaxed) & std::numeric_limits<int32_t>::max();
}

RemotingCommand::RemotingCommand(int32_t code, CommandCustomHeader* customHeader)
    : RemotingCommand(code, "CPP", MQVersion::s_CurrentVersion, createNewRequestId(), 0, "", customHeader) {}

RemotingCommand::RemotingCommand(int32_t code,
                                 const std::string& language,
                                 int32_t version,
                                 int32_t opaque,
                                 int32_t flag,
                                 const std::string& remark,
                                 CommandCustomHeader* customHeader)
    : m_code(code),
      m_language(language),
      m_version(version),
      m_opaque(opaque),
      m_flag(flag),
      m_remark(remark),
      m_customHeader(customHeader) {}

RemotingCommand::RemotingCommand(RemotingCommand&& command) {
  m_code = command.m_code;
  m_language = std::move(command.m_language);
  m_version = command.m_version;
  m_opaque = command.m_opaque;
  m_flag = command.m_flag;
  m_remark = std::move(command.m_remark);
  m_extFields = std::move(command.m_extFields);
  m_customHeader = std::move(command.m_customHeader);
  m_body = std::move(command.m_body);
}

RemotingCommand::~RemotingCommand() = default;

MemoryBlockPtr RemotingCommand::encode() {
  Json::Value root;
  root["code"] = m_code;
  root["language"] = "CPP";
  root["version"] = m_version;
  root["opaque"] = m_opaque;
  root["flag"] = m_flag;
  root["remark"] = m_remark;

  Json::Value extJson;
  for (const auto& it : m_extFields) {
    extJson[it.first] = it.second;
  }
  if (m_customHeader != nullptr) {
    // write customHeader to extFields
    m_customHeader->Encode(extJson);
  }
  root["extFields"] = extJson;

  std::string header = RemotingSerializable::toJson(root);

  uint32 headerLen = header.size();
  uint32 packageLen = 4 + headerLen;
  if (m_body != nullptr) {
    packageLen += m_body->getSize();
  }

  uint32 messageHeader[2];
  messageHeader[0] = ByteOrder::swapIfLittleEndian(packageLen);
  messageHeader[1] = ByteOrder::swapIfLittleEndian(headerLen);

  auto* package = new MemoryPool(4 + packageLen);
  package->copyFrom(messageHeader, 0, sizeof(messageHeader));
  package->copyFrom(header.data(), sizeof(messageHeader), headerLen);
  if (m_body != nullptr && m_body->getSize() > 0) {
    package->copyFrom(m_body->getData(), sizeof(messageHeader) + headerLen, m_body->getSize());
  }

  return package;
}

RemotingCommand* RemotingCommand::Decode(MemoryBlockPtr2& package) {
  // decode package: 4 bytes(headerLength) + header + body
  int packageLength = package->getSize();

  uint32 netHeaderLen;
  package->copyTo(&netHeaderLen, 0, sizeof(netHeaderLen));
  int oriHeaderLen = ByteOrder::swapIfLittleEndian(netHeaderLen);
  int headerLength = oriHeaderLen & 0xFFFFFF;

  // decode header
  const char* data = package->getData();
  const char* begin = data + 4;
  const char* end = data + 4 + headerLength;

  Json::Value object;
  try {
    object = RemotingSerializable::fromJson(begin, end);
  } catch (std::exception& e) {
    LOG_WARN_NEW("parse json failed. {}", e.what());
    THROW_MQEXCEPTION(MQClientException, "conn't parse json", -1);
  }

  int32_t code = object["code"].asInt();
  std::string language = object["language"].asString();
  int32_t version = object["version"].asInt();
  int32_t opaque = object["opaque"].asInt();
  int32_t flag = object["flag"].asInt();
  std::string remark;
  if (!object["remark"].isNull()) {
    remark = object["remark"].asString();
  }

  std::unique_ptr<RemotingCommand> cmd(new RemotingCommand(code, language, version, opaque, flag, remark, nullptr));

  if (!object["extFields"].isNull()) {
    auto extFields = object["extFields"];
    for (auto& name : extFields.getMemberNames()) {
      auto& value = extFields[name];
      if (value.isString()) {
        cmd->m_extFields[name] = value.asString();
      }
    }
  }

  // decode body
  int bodyLength = packageLength - 4 - headerLength;
  if (bodyLength > 0) {
    auto* body = new MemoryView(package, 4 + headerLength);
    cmd->setBody(body);
  }

  LOG_DEBUG_NEW("code:{}, language:{}, version:{}, opaque:{}, flag:{}, remark:{}, headLen:{}, bodyLen:{}", code,
                language, version, opaque, flag, remark, headerLength, bodyLength);

  return cmd.release();
}

int32_t RemotingCommand::getCode() const {
  return m_code;
}

void RemotingCommand::setCode(int32_t code) {
  m_code = code;
}

int32_t RemotingCommand::getVersion() const {
  return m_version;
}

int32_t RemotingCommand::getOpaque() const {
  return m_opaque;
}

void RemotingCommand::setOpaque(int32_t opaque) {
  m_opaque = opaque;
}

int32_t RemotingCommand::getFlag() const {
  return m_flag;
}

const std::string& RemotingCommand::getRemark() const {
  return m_remark;
}

void RemotingCommand::setRemark(const std::string& mark) {
  m_remark = mark;
}

bool RemotingCommand::isResponseType() {
  int bits = 1 << RPC_TYPE;
  return (m_flag & bits) == bits;
}

void RemotingCommand::markResponseType() {
  int bits = 1 << RPC_TYPE;
  m_flag |= bits;
}

bool RemotingCommand::isOnewayRPC() {
  int bits = 1 << RPC_ONEWAY;
  return (m_flag & bits) == bits;
}

void RemotingCommand::markOnewayRPC() {
  int bits = 1 << RPC_ONEWAY;
  m_flag |= bits;
}

void RemotingCommand::addExtField(const std::string& key, const std::string& value) {
  m_extFields[key] = value;
}

CommandCustomHeader* RemotingCommand::readCustomHeader() const {
  return m_customHeader.get();
}

MemoryBlockPtr2 RemotingCommand::getBody() {
  return m_body;
}

void RemotingCommand::setBody(MemoryBlock* body) {
  m_body.reset(body);
}

void RemotingCommand::setBody(MemoryBlockPtr2 body) {
  m_body = body;
}

void RemotingCommand::setBody(const std::string& body) {
  m_body.reset(new MemoryPool(body.data(), body.size()));
}

std::string RemotingCommand::toString() const {
  std::stringstream ss;
  ss << "code:" << m_code << ", opaque:" << m_opaque << ", flag:" << m_flag << ", body.size:" << m_body->getSize();
  return ss.str();
}

}  // namespace rocketmq
