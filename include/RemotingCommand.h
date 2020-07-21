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
#ifndef ROCKETMQ_REMOTINGCOMMAND_H_
#define ROCKETMQ_REMOTINGCOMMAND_H_

#include <exception>  // std::exception
#include <map>        // std::map
#include <memory>     // std::unique_ptr, std::shared_ptr
#include <string>     // std::string
#include <typeindex>  // std::type_index

#include "ByteArray.h"
#include "CommandCustomHeader.h"
#include "MQException.h"

namespace rocketmq {

/**
 * RemotingCommand - rocketmq rpc protocol
 */
class ROCKETMQCLIENT_API RemotingCommand {
 public:
  static int32_t createNewRequestId();

 public:
  RemotingCommand() : code_(0) {}
  RemotingCommand(int32_t code, CommandCustomHeader* customHeader = nullptr);
  RemotingCommand(int32_t code, const std::string& remark, CommandCustomHeader* customHeader = nullptr);
  RemotingCommand(int32_t code,
                  const std::string& language,
                  int32_t version,
                  int32_t opaque,
                  int32_t flag,
                  const std::string& remark,
                  CommandCustomHeader* customHeader);

  RemotingCommand(RemotingCommand&& command);

  virtual ~RemotingCommand();

 public:
  bool isResponseType();
  void markResponseType();

  bool isOnewayRPC();
  void markOnewayRPC();

  CommandCustomHeader* readCustomHeader() const;

 public:
  ByteArrayRef encode() const;

  template <class H>
  H* decodeCommandCustomHeader(bool useCache = true);

  static RemotingCommand* Decode(ByteArrayRef array, bool hasPackageLength = false);

  std::string toString() const;

 public:
  inline int32_t code() const { return code_; }
  inline void set_code(int32_t code) { code_ = code; }

  inline int32_t version() const { return version_; }

  inline int32_t opaque() const { return opaque_; }
  inline void set_opaque(int32_t opaque) { opaque_ = opaque; }

  inline int32_t flag() const { return flag_; }

  inline const std::string& remark() const { return remark_; }
  inline void set_remark(const std::string& remark) { remark_ = remark; }

  inline void set_ext_field(const std::string& name, const std::string& value) { ext_fields_[name] = value; }

  inline ByteArrayRef body() const { return body_; }
  inline void set_body(ByteArrayRef body) { body_ = std::move(body); }
  inline void set_body(const std::string& body) { body_ = stoba(body); }
  inline void set_body(std::string&& body) { body_ = stoba(std::move(body)); }

 private:
  int32_t code_;
  std::string language_;
  int32_t version_;
  int32_t opaque_;
  int32_t flag_;
  std::string remark_;
  std::map<std::string, std::string> ext_fields_;

  std::unique_ptr<CommandCustomHeader> custom_header_;  // transient

  ByteArrayRef body_;  // transient
};

template <class H>
H* RemotingCommand::decodeCommandCustomHeader(bool useCache) {
  if (useCache) {
    auto* cache = custom_header_.get();
    if (cache != nullptr && std::type_index(typeid(*cache)) == std::type_index(typeid(H))) {
      return static_cast<H*>(custom_header_.get());
    }
  }

  try {
    H* header = H::Decode(ext_fields_);
    custom_header_.reset(header);
    return header;
  } catch (std::exception& e) {
    THROW_MQEXCEPTION(RemotingCommandException, e.what(), -1);
  }
}

}  // namespace rocketmq

#endif  // ROCKETMQ_REMOTINGCOMMAND_H_
