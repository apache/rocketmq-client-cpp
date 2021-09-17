#pragma once

#include "NameServerResolver.h"
#include "gmock/gmock.h"

ROCKETMQ_NAMESPACE_BEGIN

class NameServerResolverMock : public NameServerResolver {
public:
  MOCK_METHOD(void, start, (), (override));

  MOCK_METHOD(void, shutdown, (), (override));

  MOCK_METHOD(std::string, next, (), (override));

  MOCK_METHOD(std::string, current, (), (override));

  MOCK_METHOD((std::vector<std::string>), resolve, (), (override));
};

ROCKETMQ_NAMESPACE_END