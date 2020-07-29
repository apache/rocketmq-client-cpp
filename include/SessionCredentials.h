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
#ifndef ROCKETMQ_SESSIONCREDENTIALS_H_
#define ROCKETMQ_SESSIONCREDENTIALS_H_

#include <string>  // std::string

#include "RocketMQClient.h"

namespace rocketmq {

class ROCKETMQCLIENT_API SessionCredentials {
 public:
  SessionCredentials() : auth_channel_("ALIYUN") {}
  SessionCredentials(const std::string& accessKey, const std::string& secretKey, const std::string& authChannel)
      : access_key_(accessKey), secret_key_(secretKey), auth_channel_(authChannel) {}
  SessionCredentials(const SessionCredentials& other)
      : access_key_(other.access_key_),
        secret_key_(other.secret_key_),
        signature_(other.signature_),
        signature_method_(other.signature_method_),
        auth_channel_(other.auth_channel_) {}

  ~SessionCredentials() = default;

  bool isValid() const { return !access_key_.empty() && !secret_key_.empty() && !auth_channel_.empty(); }

  inline const std::string& access_key() const { return access_key_; }
  inline void set_access_key(const std::string& accessKey) { access_key_ = accessKey; }

  inline const std::string& secret_key() const { return secret_key_; }
  inline void set_secret_key(const std::string& secretKey) { secret_key_ = secretKey; }

  inline const std::string& signature() const { return signature_; }
  inline void set_signature(const std::string& signature) { signature_ = signature; }

  inline const std::string& signature_method() const { return signature_method_; }
  inline void set_signature_method(const std::string& signatureMethod) { signature_method_ = signatureMethod; }

  inline const std::string& auth_channel() const { return auth_channel_; }
  inline void set_auth_channel(const std::string& channel) { auth_channel_ = channel; }

 private:
  std::string access_key_;
  std::string secret_key_;
  std::string signature_;
  std::string signature_method_;
  std::string auth_channel_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_SESSIONCREDENTIALS_H_
