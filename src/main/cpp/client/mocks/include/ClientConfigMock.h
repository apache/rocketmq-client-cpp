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

#include "ClientConfig.h"
#include "gmock/gmock.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientConfigMock : virtual public ClientConfig {
public:
  ~ClientConfigMock() override = default;
  MOCK_METHOD(const std::string&, region, (), (const override));
  MOCK_METHOD(const std::string&, serviceName, (), (const override));
  MOCK_METHOD(const std::string&, resourceNamespace, (), (const overide));
  MOCK_METHOD(CredentialsProviderPtr, credentialsProvider, (), (override));
  MOCK_METHOD(const std::string&, tenantId, (), (const override));
  MOCK_METHOD(absl::Duration, getIoTimeout, (), (const override));
  MOCK_METHOD(absl::Duration, getLongPollingTimeout, (), (const override));
  MOCK_METHOD(const std::string&, getGroupName, (), (const override));
  MOCK_METHOD(std::string, clientId, (), (const override));
  MOCK_METHOD(bool, isTracingEnabled, (), (const override));
};

ROCKETMQ_NAMESPACE_END