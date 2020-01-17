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
#ifndef __SESSION_CREDENTIALS_H__
#define __SESSION_CREDENTIALS_H__

#include <string>

#include "RocketMQClient.h"

namespace rocketmq {

class ROCKETMQCLIENT_API SessionCredentials {
 public:
  static const std::string AccessKey;
  static const std::string SecretKey;
  static const std::string Signature;
  static const std::string SignatureMethod;
  static const std::string ONSChannelKey;

  SessionCredentials() : authChannel_("ALIYUN") {}
  SessionCredentials(const std::string& accessKey, const std::string& secretKey, const std::string& authChannel)
      : accessKey_(accessKey), secretKey_(secretKey), authChannel_(authChannel) {}

  ~SessionCredentials() = default;

  const std::string& getAccessKey() const { return accessKey_; }
  void setAccessKey(const std::string& accessKey) { accessKey_ = accessKey; }

  const std::string& getSecretKey() const { return secretKey_; }
  void setSecretKey(const std::string& secretKey) { secretKey_ = secretKey; }

  const std::string& getSignature() const { return signature_; }
  void setSignature(const std::string& signature) { signature_ = signature; }

  const std::string& getSignatureMethod() const { return signatureMethod_; }
  void setSignatureMethod(const std::string& signatureMethod) { signatureMethod_ = signatureMethod; }

  const std::string& getAuthChannel() const { return authChannel_; }
  void setAuthChannel(const std::string& channel) { authChannel_ = channel; }

  bool isValid() const { return !accessKey_.empty() && !secretKey_.empty() && !authChannel_.empty(); }

 private:
  std::string accessKey_;
  std::string secretKey_;
  std::string signature_;
  std::string signatureMethod_;
  std::string authChannel_;
};

}  // namespace rocketmq

#endif  // __SESSION_CREDENTIALS_H__
