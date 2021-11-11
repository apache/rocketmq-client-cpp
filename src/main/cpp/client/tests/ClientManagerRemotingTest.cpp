#include <chrono>
#include <memory>

#include "absl/synchronization/mutex.h"
#include "gtest/gtest.h"

#include "ClientManagerImpl.h"
#include "rocketmq/Logger.h"
#include "rocketmq/RocketMQ.h"
#include "rocketmq/TransportType.h"

ROCKETMQ_NAMESPACE_BEGIN

class ClientManagerRemotingTest : public testing::Test {
public:
  void SetUp() override {
    Logger& logger = getLogger();
    logger.setLevel(Level::Debug);
    logger.setConsoleLevel(Level::Debug);
    logger.init();

    client_config_.transportType(TransportType::Remoting);
    client_manager_ = std::make_shared<ClientManagerImpl>(client_config_);
    client_manager_->start();
  }

  void TearDown() override {
    client_manager_.reset();
  }

protected:
  std::string group_{"TestGroup"};
  ClientConfigImpl client_config_{group_};
  std::shared_ptr<ClientManagerImpl> client_manager_;
};

TEST_F(ClientManagerRemotingTest, testLifecycle) {
}

TEST_F(ClientManagerRemotingTest, testQueryRoute) {
  std::string target_host = "11.163.70.118:9876";
  Metadata metadata;

  QueryRouteRequest request;
  request.mutable_topic()->set_name("zhanhui-test");

  bool callback_invoked = false;
  absl::Mutex callback_mtx;
  absl::CondVar callback_cv;

  auto callback = [&](const std::error_code& ec, const TopicRouteDataPtr& route) {
    callback_invoked = true;
    {
      absl::MutexLock lk(&callback_mtx);
      callback_cv.SignalAll();
    }
  };

  client_manager_->resolveRoute(target_host, metadata, request, std::chrono::seconds(3), callback);

  {
    absl::MutexLock lk(&callback_mtx);
    callback_cv.WaitWithTimeout(&callback_mtx, absl::Seconds(10));
  }
  EXPECT_TRUE(callback_invoked);
}

ROCKETMQ_NAMESPACE_END