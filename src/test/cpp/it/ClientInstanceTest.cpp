#include "ClientInstance.h"
#include "ClientManager.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientInstanceTest : public testing::Test {
protected:
  std::string name_server_{"grpc.dev:9876"};
};

ROCKETMQ_NAMESPACE_END
