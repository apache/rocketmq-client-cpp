#include <chrono>
#include <memory>
#include <string>

#include "ClientImpl.h"
#include "ClientManagerFactory.h"
#include "ClientManagerMock.h"
#include "DynamicNameServerResolver.h"
#include "HttpClientMock.h"
#include "NameServerResolverMock.h"
#include "SchedulerImpl.h"
#include "TopAddressing.h"
#include "rocketmq/RocketMQ.h"

#include "gtest/gtest.h"

ROCKETMQ_NAMESPACE_BEGIN

class TestClientImpl : public ClientImpl, public std::enable_shared_from_this<TestClientImpl> {
public:
  TestClientImpl(std::string group) : ClientImpl(std::move(group)) {}

  std::shared_ptr<ClientImpl> self() override { return shared_from_this(); }

  void prepareHeartbeatData(HeartbeatRequest& request) override {}
};

class ClientImplTest : public testing::Test {
public:
  void SetUp() override {
    grpc_init();
    name_server_resolver_ = std::make_shared<DynamicNameServerResolver>(endpoint_, std::chrono::seconds(1));
    scheduler_.start();
    client_manager_ = std::make_shared<testing::NiceMock<ClientManagerMock>>();
    ClientManagerFactory::getInstance().addClientManager(resource_namespace_, client_manager_);
    ON_CALL(*client_manager_, getScheduler).WillByDefault(testing::ReturnRef(scheduler_));
    client_ = std::make_shared<TestClientImpl>(group_);
    client_->withNameServerResolver(name_server_resolver_);
  }

  void TearDown() override {
    grpc_shutdown();
    scheduler_.shutdown();
  }

protected:
  std::string endpoint_{"http://jmenv.tbsite.net:8080/rocketmq/nsaddr"};
  std::string resource_namespace_{"mq://test"};
  std::string group_{"Group-0"};
  std::shared_ptr<testing::NiceMock<ClientManagerMock>> client_manager_;
  SchedulerImpl scheduler_;
  std::shared_ptr<TestClientImpl> client_;
  std::shared_ptr<DynamicNameServerResolver> name_server_resolver_;
};

TEST_F(ClientImplTest, testBasic) {

  auto http_client = absl::make_unique<HttpClientMock>();

  std::string once{"10.0.0.1:9876"};
  std::string then{"10.0.0.1:9876;10.0.0.2:9876"};
  std::multimap<std::string, std::string> header;

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  int http_status = 200;
  auto once_cb =
      [&](HttpProtocol protocol, const std::string& host, std::uint16_t port, const std::string& path,
          const std::function<void(int, const std::multimap<std::string, std::string>&, const std::string&)>& cb) {
        cb(http_status, header, once);
      };
  auto then_cb =
      [&](HttpProtocol protocol, const std::string& host, std::uint16_t port, const std::string& path,
          const std::function<void(int, const std::multimap<std::string, std::string>&, const std::string&)>& cb) {
        cb(http_status, header, then);
        absl::MutexLock lk(&mtx);
        completed = true;
        cv.SignalAll();
      };

  EXPECT_CALL(*http_client, get).WillOnce(testing::Invoke(once_cb)).WillRepeatedly(testing::Invoke(then_cb));
  name_server_resolver_->injectHttpClient(std::move(http_client));

  client_->resourceNamespace(resource_namespace_);
  client_->start();
  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);

  client_->shutdown();
}

ROCKETMQ_NAMESPACE_END