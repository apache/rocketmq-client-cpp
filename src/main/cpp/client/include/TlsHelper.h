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
#pragma once

#include "rocketmq/RocketMQ.h"
#include <string>
#include <utility>

#include "grpcpp/security/tls_credentials_options.h"
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <openssl/x509.h>

#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

class TlsHelper {

public:
  static std::string sign(const std::string& access_secret, const std::string& content);

  static const char* CA;

  static const char* client_certificate_chain;

  static const char* client_private_key;
};

class TlsServerAuthorizationChecker : public grpc::experimental::TlsServerAuthorizationCheckInterface {
public:
  int Schedule(grpc::experimental::TlsServerAuthorizationCheckArg* arg) override {
    if (nullptr == arg) {
      return 0;
    }

    arg->set_success(1);
    arg->set_status(GRPC_STATUS_OK);
    return 0;
  }

  ~TlsServerAuthorizationChecker() override {
    SPDLOG_DEBUG("~TlsServerAuthorizationChecker() invoked");
  }
};

ROCKETMQ_NAMESPACE_END