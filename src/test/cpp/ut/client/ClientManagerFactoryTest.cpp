#include "gtest/gtest.h"
#include "ClientManagerFactory.h"
#include "ClientConfigMock.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientManagerFactoryTest : public testing::Test {
public:
  void SetUp() override {}

  void TearDown() override {}

protected:
  testing::NiceMock<ClientConfigMock> client_config_;
  std::string arn_{"arn:mq://test"};
};

TEST_F(ClientManagerFactoryTest, testGetClientManager) {
  EXPECT_CALL(client_config_, arn).Times(testing::AtLeast(1)).WillRepeatedly(testing::ReturnRef(arn_));
  ClientManagerPtr client_manager = ClientManagerFactory::getInstance().getClientManager(client_config_);
  EXPECT_TRUE(client_manager);
}

ROCKETMQ_NAMESPACE_END