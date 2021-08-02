#include "ClientManagerImpl.h"
#include "RpcClientMock.h"
#include "gtest/gtest.h"
#include <apache/rocketmq/v1/definition.pb.h>
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN

class ClientManagerTest : public testing::Test {
public:
  void SetUp() override {
    client_manager_ = std::make_shared<ClientManagerImpl>(arn_);
    client_manager_->start();
    rpc_client_ = std::make_shared<testing::NiceMock<RpcClientMock>>();
    ON_CALL(*rpc_client_, ok).WillByDefault(testing::Return(true));
    client_manager_->addRpcClient(target_host_, rpc_client_);
  }

  void TearDown() override { client_manager_->shutdown(); }

protected:
  std::string arn_{"arn:mq://test"};
  std::string topic_{"TestTopic"};
  std::string target_host_{"ipv4:10.0.0.0:10911"};
  std::shared_ptr<ClientManagerImpl> client_manager_;
  std::shared_ptr<testing::NiceMock<RpcClientMock>> rpc_client_;
  absl::Duration io_timeout_{absl::Seconds(3)};
};

TEST_F(ClientManagerTest, testBasic) {
  // Ensure that start/shutdown works well.
}

TEST_F(ClientManagerTest, testResolveRoute) {
  auto rpc_cb = [](const QueryRouteRequest& request, InvocationContext<QueryRouteResponse>* invocation_context) {
    auto partition = new rmq::Partition();
    partition->mutable_topic()->set_arn(request.topic().arn());
    partition->mutable_topic()->set_name(request.topic().name());
    partition->mutable_broker()->set_name("broker-0");
    partition->mutable_broker()->set_id(0);
    auto address = new rmq::Address();
    address->set_host("10.0.0.1");
    address->set_port(10911);
    partition->mutable_broker()->mutable_endpoints()->set_scheme(rmq::AddressScheme::IPv4);
    partition->mutable_broker()->mutable_endpoints()->mutable_addresses()->AddAllocated(address);
    invocation_context->response.mutable_partitions()->AddAllocated(partition);

    invocation_context->onCompletion(true);
  };
  EXPECT_CALL(*rpc_client_, asyncQueryRoute).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(rpc_cb));

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  Metadata metadata;
  QueryRouteRequest request;
  request.mutable_topic()->set_arn(arn_);
  request.mutable_topic()->set_name(topic_);
  auto callback = [&](bool, const TopicRouteDataPtr&) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
  };
  client_manager_->resolveRoute(target_host_, metadata, request, absl::ToChronoMilliseconds(io_timeout_), callback);
  {
    absl::MutexLock lk(&mtx);
    cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
  }
  EXPECT_TRUE(completed);
}

ROCKETMQ_NAMESPACE_END