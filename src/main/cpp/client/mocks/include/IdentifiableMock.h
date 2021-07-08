#pragma once

#include "Identifiable.h"
#include "gmock/gmock.h"

ROCKETMQ_NAMESPACE_BEGIN

class IdentifiableMock : public Identifiable {
public:
  ~IdentifiableMock() override = default;
  MOCK_METHOD(CredentialsProviderPtr, credentialsProvider, (), (override));
  MOCK_METHOD(const std::string&, tenantId, (), (const override));
};

ROCKETMQ_NAMESPACE_END