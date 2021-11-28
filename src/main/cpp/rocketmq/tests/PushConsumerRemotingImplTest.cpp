#include "gtest/gtest.h"
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#include "PushConsumerRemotingImpl.h"
#include "StaticNameServerResolver.h"
#include "LoggerImpl.h"

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
  std::string name_server_{"192.168.31.224:9876"};
  std::shared_ptr<PushConsumerRemotingImpl> consumer_;
};

TEST_F(PushConsumerRemotingImplTest, test_start_shutdown) {
  consumer_->start();
  consumer_->shutdown();
}

TEST_F(PushConsumerRemotingImplTest, testGetFilterExpression) {
  auto optional = consumer_->getFilterExpression(topic_);
  ASSERT_FALSE(optional.has_value());

  consumer_->subscribe(topic_, "*");
  optional = consumer_->getFilterExpression(topic_);
  ASSERT_TRUE(optional.has_value());
  SPDLOG_INFO(optional.value().content_);
}

TEST_F(PushConsumerRemotingImplTest, testRouteFetchAtStart) {
  consumer_->subscribe(topic_, "*");
  consumer_->start();
  SPDLOG_INFO("Start to shutdown consumer");
  std::this_thread::sleep_for(std::chrono::seconds(10));
  consumer_->shutdown();
}

ROCKETMQ_NAMESPACE_END