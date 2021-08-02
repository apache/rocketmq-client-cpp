#pragma once

#include "ClientConfig.h"
#include "gmock/gmock.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientConfigMock : virtual public ClientConfig {
public:
  ~ClientConfigMock() override = default;
  MOCK_METHOD(const std::string&, region, (), (const override));
  MOCK_METHOD(const std::string&, serviceName, (), (const override));
  MOCK_METHOD(const std::string&, arn, (), (const overide));
  MOCK_METHOD(CredentialsProviderPtr, credentialsProvider, (), (override));
  MOCK_METHOD(const std::string&, tenantId, (), (const override));
  MOCK_METHOD(absl::Duration, getIoTimeout, (), (const override));
  MOCK_METHOD(absl::Duration, getLongPollingTimeout, (), (const override));
  MOCK_METHOD(const std::string&, getGroupName, (), (const override));
  MOCK_METHOD(std::string, clientId, (), (const override));
};

ROCKETMQ_NAMESPACE_END