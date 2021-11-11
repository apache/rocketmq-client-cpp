#include "ClientManagerImpl.h"
#include "rocketmq/RocketMQ.h"
#include "rocketmq/TransportType.h"

#include "gtest/gtest.h"
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN

class ClientManagerRemotingTest : public testing::Test {
public:
  void SetUp() override {

    client_config_.transportType(TransportType::Remoting);
    client_manager_ = std::make_shared<ClientManagerImpl>(client_config_);

    client_manager_->start();
  }

  void TearDown() override {
    client_manager_->shutdown();
    client_manager_.reset();
  }

protected:
  std::string group_{"TestGroup"};
  ClientConfigImpl client_config_{group_};
  std::shared_ptr<ClientManagerImpl> client_manager_;
};

TEST_F(ClientManagerRemotingTest, testLifecycle) {
}

ROCKETMQ_NAMESPACE_END