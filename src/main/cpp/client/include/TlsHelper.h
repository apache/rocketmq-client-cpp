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

#include <string>
#include <utility>

#include "grpcpp/client_context.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/tls_credentials_options.h"
#include "openssl/x509.h"

#include "LoggerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

class TlsHelper {

public:
  static std::string sign(const std::string& access_secret, const std::string& content);

  static const char* CA;

  static const char* client_certificate_chain;

  static const char* client_private_key;
};

ROCKETMQ_NAMESPACE_END