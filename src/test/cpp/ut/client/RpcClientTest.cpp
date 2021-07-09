#include "RpcClientMock.h"
#include <grpcpp/impl/grpc_library.h>
#include <gtest/gtest.h>

ROCKETMQ_NAMESPACE_BEGIN

namespace ut {

class RpcClientTest : public testing::Test {

public:
  void SetUp() override { grpc::internal::GrpcLibraryInitializer initializer; }

  static void mockQueryRouteInfo(const QueryRouteRequest& request,
                                 InvocationContext<QueryRouteResponse>* invocation_context) {
    invocation_context->response.mutable_common()->mutable_status()->set_code(google::rpc::Code::OK);
    for (int i = 0; i < 3; ++i) {
      auto partition = new rmq::Partition;
      partition->mutable_topic()->set_name(request.topic().name());
      partition->mutable_broker()->set_name(fmt::format("broker-{}", i));
      partition->mutable_broker()->set_id(0);
      auto endpoint = partition->mutable_broker()->mutable_endpoints();
      auto address = new rmq::Address;
      address->set_host(fmt::format("10.0.0.{}", i));
      address->set_port(10911);
      endpoint->mutable_addresses()->AddAllocated(address);
      invocation_context->response.mutable_partitions()->AddAllocated(partition);
    }

    invocation_context->onCompletion(true);
  }
};

TEST_F(RpcClientTest, testMockedGetRouteInfo) {
  RpcClientMock rpc_client_mock;
  ON_CALL(rpc_client_mock, asyncQueryRoute(testing::_, testing::_)).WillByDefault(testing::Invoke(mockQueryRouteInfo));
  std::string topic = "sample_topic";
  QueryRouteRequest request;
  request.mutable_topic()->set_name(topic);
  absl::flat_hash_map<std::string, std::string> metadata;
  auto invocation_context = new InvocationContext<QueryRouteResponse>();
  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;
  auto callback = [&](const InvocationContext<QueryRouteResponse>* invocation_context) {
    EXPECT_TRUE(invocation_context->status.ok());
    EXPECT_EQ(google::rpc::Code::OK, invocation_context->response.common().status().code());
    EXPECT_EQ(3, invocation_context->response.partitions().size());
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
  };
  invocation_context->callback = callback;
  rpc_client_mock.asyncQueryRoute(request, invocation_context);
  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }
}

} // namespace ut
ROCKETMQ_NAMESPACE_END