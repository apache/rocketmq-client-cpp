#include "gtest/gtest.h"
#include <memory>
#include <string>

#include "PushConsumerRemotingImpl.h"
#include "StaticNameServerResolver.h"

ROCKETMQ_NAMESPACE_BEGIN

class PushConsumerRemotingImplTest : public testing::Test {
public:
  void SetUp() override {
    consumer_ = std::make_shared<PushConsumerRemotingImpl>(group_);
    consumer_->withNameServerResolver(std::make_shared<StaticNameServerResolver>(name_server_));
  }

  void TearDown() override {
    consumer_.reset();
  }

protected:
  std::string group_{"CG_TestGroup"};
  std::string topic_{"zhanhui-test"};
  std::string name_server_{"11.163.70.118:9876"};
  std::shared_ptr<PushConsumerRemotingImpl> consumer_;
};

TEST_F(PushConsumerRemotingImplTest, test_start_shutdown) {
  consumer_->start();
  consumer_->shutdown();
}

TEST_F(PushConsumerRemotingImplTest, testGetFilterExpression) {
  auto optional = consumer_->getFilterExpression(topic_);
  ASSERT_FALSE(optional.has_value());
}

TEST_F(PushConsumerRemotingImplTest, testRebalance) {
}

ROCKETMQ_NAMESPACE_END