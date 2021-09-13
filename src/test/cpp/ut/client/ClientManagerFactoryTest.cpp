#include "ClientManagerFactory.h"
#include "ClientConfigMock.h"
#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientManagerFactoryTest : public testing::Test {
public:
  void SetUp() override {}

  void TearDown() override {}

protected:
  testing::NiceMock<ClientConfigMock> client_config_;
  std::string resource_namespace_{"mq://test"};
};

TEST_F(ClientManagerFactoryTest, testGetClientManager) {
  EXPECT_CALL(client_config_, resourceNamespace).Times(testing::AtLeast(1)).WillRepeatedly(testing::ReturnRef(resource_namespace_));
  ClientManagerPtr client_manager = ClientManagerFactory::getInstance().getClientManager(client_config_);
  EXPECT_TRUE(client_manager);
}

ROCKETMQ_NAMESPACE_END