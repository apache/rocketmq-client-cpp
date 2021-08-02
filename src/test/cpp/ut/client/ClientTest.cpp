#include "ClientMock.h"
#include "rocketmq/RocketMQ.h"
#include "gtest/gtest.h"
#include <iostream>
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN

class ClientTest : public testing::Test {
public:
  void SetUp() override {
    client_ = std::make_shared<testing::NiceMock<ClientMock>>();
    ON_CALL(*client_, active).WillByDefault(testing::Invoke([]() {
      std::cout << "active() is invoked" << std::endl;
      return true;
    }));
  }

  void TearDown() override {}

protected:
  std::shared_ptr<testing::NiceMock<ClientMock>> client_;
};

TEST_F(ClientTest, testActive) { EXPECT_TRUE(client_->active()); }

ROCKETMQ_NAMESPACE_END