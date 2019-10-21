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

#ifndef __SESSIONCREDENTIALS_H__
#define __SESSIONCREDENTIALS_H__

#include "RocketMQClient.h"

namespace rocketmq {

class SessionCredentials {
 public:
  static const std::string AccessKey;
  static const std::string SecretKey;
  static const std::string Signature;
  static const std::string SignatureMethod;
  static const std::string ONSChannelKey;

  SessionCredentials(std::string input_accessKey, std::string input_secretKey, const std::string& input_authChannel)
      : accessKey(input_accessKey), secretKey(input_secretKey), authChannel(input_authChannel) {}
  SessionCredentials() : authChannel("ALIYUN") {}
  ~SessionCredentials() {}

  std::string getAccessKey() const { return accessKey; }

  void setAccessKey(std::string input_accessKey) { accessKey = input_accessKey; }

  std::string getSecretKey() const { return secretKey; }

  void setSecretKey(std::string input_secretKey) { secretKey = input_secretKey; }

  std::string getSignature() const { return signature; }

  void setSignature(std::string input_signature) { signature = input_signature; }

  std::string getSignatureMethod() const { return signatureMethod; }

  void setSignatureMethod(std::string input_signatureMethod) { signatureMethod = input_signatureMethod; }

  std::string getAuthChannel() const { return authChannel; }

  void setAuthChannel(std::string input_channel) { authChannel = input_channel; }

  bool isValid() const {
    if (accessKey.empty() || secretKey.empty() || authChannel.empty())
      return false;

    return true;
  }

 private:
  std::string accessKey;
  std::string secretKey;
  std::string signature;
  std::string signatureMethod;
  std::string authChannel;
};
}  // namespace rocketmq
#endif
