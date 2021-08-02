#pragma once

#include "Client.h"
#include "ClientConfigMock.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientMock : virtual public Client, virtual public ClientConfigMock {
public:
  MOCK_METHOD(void, endpointsInUse, (absl::flat_hash_set<std::string>&), (override));

  MOCK_METHOD(void, heartbeat, (), (override));

  MOCK_METHOD(bool, active, (), (override));

  MOCK_METHOD(void, healthCheck, (), (override));

  MOCK_METHOD(void, schedule, (const std::string&, const std::function<void()>&, std::chrono::milliseconds),
              (override));
};

ROCKETMQ_NAMESPACE_END
