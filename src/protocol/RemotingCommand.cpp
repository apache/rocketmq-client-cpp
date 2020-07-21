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

#include <cstring>  // std::memcpy

#include <algorithm>  // std::move
#include <atomic>     // std::atomic
#include <limits>     // std::numeric_limits

#include "ByteOrder.h"
#include "ByteBuffer.hpp"
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
    : RemotingCommand(code, "", customHeader) {}

RemotingCommand::RemotingCommand(int32_t code, const std::string& remark, CommandCustomHeader* customHeader)
    : RemotingCommand(code,
                      MQVersion::CURRENT_LANGUAGE,
                      MQVersion::CURRENT_VERSION,
                      createNewRequestId(),
                      0,
                      remark,
                      customHeader) {}

RemotingCommand::RemotingCommand(int32_t code,
                                 const std::string& language,
                                 int32_t version,
                                 int32_t opaque,
                                 int32_t flag,
                                 const std::string& remark,
                                 CommandCustomHeader* customHeader)
    : code_(code),
      language_(language),
      version_(version),
      opaque_(opaque),
      flag_(flag),
      remark_(remark),
      custom_header_(customHeader) {}

RemotingCommand::RemotingCommand(RemotingCommand&& command) {
  code_ = command.code_;
  language_ = std::move(command.language_);
  version_ = command.version_;
  opaque_ = command.opaque_;
  flag_ = command.flag_;
  remark_ = std::move(command.remark_);
  ext_fields_ = std::move(command.ext_fields_);
  custom_header_ = std::move(command.custom_header_);
  body_ = std::move(command.body_);
}

RemotingCommand::~RemotingCommand() = default;

ByteArrayRef RemotingCommand::encode() const {
  Json::Value root;
  root["code"] = code_;
  root["language"] = language_;
  root["version"] = version_;
  root["opaque"] = opaque_;
  root["flag"] = flag_;
  root["remark"] = remark_;

  Json::Value ext_fields;
  for (const auto& it : ext_fields_) {
    ext_fields[it.first] = it.second;
  }
  if (custom_header_ != nullptr) {
    // write customHeader to extFields
    custom_header_->Encode(ext_fields);
  }
  root["extFields"] = ext_fields;

  // serialize header
  std::string header = RemotingSerializable::toJson(root);

  // 1> header length size
  uint32_t length = 4;
  // 2> header data length
  length += header.size();
  // 3> body data length
  if (body_ != nullptr) {
    length += body_->size();
  }

  std::unique_ptr<ByteBuffer> result(ByteBuffer::allocate(4 + length));

  // length
  result->putInt(length);
  // header length
  result->putInt((uint32_t)header.size());
  // header data
  result->put(ByteArray((char*)header.data(), header.size()));
  // body data;
  if (body_ != nullptr) {
    result->put(*body_);
  }

  // result->flip();

  return result->byte_array();
}

static inline int32_t getHeaderLength(int32_t length) {
  return length & 0x00FFFFFF;
}

static RemotingCommand* Decode(ByteBuffer& byteBuffer, bool hasPackageLength) {
  // decode package: [4 bytes(packageLength) +] 4 bytes(headerLength) + header + body

  int32_t length = byteBuffer.limit();
  if (hasPackageLength) {
    // skip package length
    (void)byteBuffer.getInt();
    length -= 4;
  }

  // decode header

  int32_t oriHeaderLen = byteBuffer.getInt();
  int32_t headerLength = getHeaderLength(oriHeaderLen);

  // temporary ByteArray
  ByteArray headerData(byteBuffer.array() + byteBuffer.arrayOffset() + byteBuffer.position(), headerLength);
  byteBuffer.position(byteBuffer.position() + headerLength);

  Json::Value object;
  try {
    object = RemotingSerializable::fromJson(headerData);
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
        cmd->set_ext_field(name, value.asString());
      }
    }
  }

  // decode body

  int32_t bodyLength = length - 4 - headerLength;
  if (bodyLength > 0) {
    // slice ByteArray of byteBuffer to avoid copy data.
    ByteArrayRef bodyData =
        slice(byteBuffer.byte_array(), byteBuffer.arrayOffset() + byteBuffer.position(), bodyLength);
    byteBuffer.position(byteBuffer.position() + bodyLength);
    cmd->set_body(std::move(bodyData));
  }

  LOG_DEBUG_NEW("code:{}, language:{}, version:{}, opaque:{}, flag:{}, remark:{}, headLen:{}, bodyLen:{}", code,
                language, version, opaque, flag, remark, headerLength, bodyLength);

  return cmd.release();
}

RemotingCommand* RemotingCommand::Decode(ByteArrayRef array, bool hasPackageLength) {
  std::unique_ptr<ByteBuffer> byteBuffer(ByteBuffer::wrap(std::move(array)));
  return rocketmq::Decode(*byteBuffer, hasPackageLength);
}

bool RemotingCommand::isResponseType() {
  int bits = 1 << RPC_TYPE;
  return (flag_ & bits) == bits;
}

void RemotingCommand::markResponseType() {
  int bits = 1 << RPC_TYPE;
  flag_ |= bits;
}

bool RemotingCommand::isOnewayRPC() {
  int bits = 1 << RPC_ONEWAY;
  return (flag_ & bits) == bits;
}

void RemotingCommand::markOnewayRPC() {
  int bits = 1 << RPC_ONEWAY;
  flag_ |= bits;
}

CommandCustomHeader* RemotingCommand::readCustomHeader() const {
  return custom_header_.get();
}

std::string RemotingCommand::toString() const {
  std::stringstream ss;
  ss << "code:" << code_ << ", opaque:" << opaque_ << ", flag:" << flag_ << ", body.size:" << body_->size();
  return ss.str();
}

}  // namespace rocketmq
