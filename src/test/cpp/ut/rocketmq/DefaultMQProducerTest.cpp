#include "rocketmq/DefaultMQProducer.h"

#include "MQClientTest.h"
#include "ProducerImpl.h"
#include "rocketmq/CredentialsProvider.h"
#include "rocketmq/MQSelector.h"
#include <memory>
#include <mutex>
#include <utility>

ROCKETMQ_NAMESPACE_BEGIN

class DefaultMQProducerUnitTest : public MQClientTest {
public:
  DefaultMQProducerUnitTest() : MQClientTest() {
    credentials_provider_ = std::make_shared<StaticCredentialsProvider>(access_key_, access_secret_);
  }

  void SetUp() override {
    MQClientTest::SetUp();
    // More set-up
    rpc_client_broker_ = std::make_shared<testing::NiceMock<RpcClientMock>>();

    ON_CALL(*rpc_client_broker_, needHeartbeat()).WillByDefault(testing::Return(true));

    ON_CALL(*rpc_client_broker_, ok()).WillByDefault(testing::Invoke([this]() { return client_ok_; }));

    ON_CALL(*rpc_client_broker_, asyncHealthCheck)
        .WillByDefault(testing::Invoke(
            [this](const HealthCheckRequest& request, InvocationContext<HealthCheckResponse>* invocation_context) {
              invocation_context->response.mutable_common()->mutable_status()->set_code(ok_);
              invocation_context->onCompletion(true);
            }));

    ON_CALL(*rpc_client_broker_, asyncHeartbeat(testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            [this](const HeartbeatRequest& request, InvocationContext<HeartbeatResponse>* invocation_context) {
              invocation_context->response.mutable_common()->mutable_status()->set_code(ok_);
              invocation_context->onCompletion(true);
            }));

    ON_CALL(*rpc_client_broker_, asyncSend)
        .WillByDefault(testing::Invoke(
            [this](const SendMessageRequest& request, InvocationContext<SendMessageResponse>* invocation_context) {
              invocation_context->response.mutable_common()->mutable_status()->set_code(ok_);
              invocation_context->response.set_message_id(message_id_);
              invocation_context->onCompletion(true);
            }));

    const char* address_format = "ipv4:10.0.0.{}:10911";
    for (int i = 0; i < partition_num_ / avg_partition_per_host_; ++i) {
      std::string&& address = fmt::format(address_format, i);
      client_instance_->addRpcClient(address, rpc_client_broker_);
    }
  }

  void TearDown() override {
    rpc_client_broker_.reset();
    MQClientTest::TearDown();
  }

protected:
  absl::flat_hash_map<std::string, std::string> metadata_;
  std::shared_ptr<testing::NiceMock<RpcClientMock>> rpc_client_broker_;
  std::string message_id_{"msg_id_0"};
  std::string access_key_{"access_key"};
  std::string access_secret_{"access_secret"};
  std::shared_ptr<CredentialsProvider> credentials_provider_;
};

TEST_F(DefaultMQProducerUnitTest, testBasicSetUp) {
  QueryRouteRequest request;
  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;
  auto invocation_context = new InvocationContext<QueryRouteResponse>();
  auto callback = [&](const InvocationContext<QueryRouteResponse>* invocation_context) {
    ASSERT_TRUE(invocation_context->status.ok());
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
  };
  invocation_context->callback = callback;
  rpc_client_ns_->asyncQueryRoute(request, invocation_context);
  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }

  completed = false;
  SendMessageRequest send_message_request;
  auto send_message_invocation_context = new InvocationContext<SendMessageResponse>();
  auto send_callback = [&](const InvocationContext<SendMessageResponse>* invocation_context) {
    ASSERT_TRUE(invocation_context->response.message_id() == message_id_);
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
  };
  send_message_invocation_context->callback = send_callback;

  rpc_client_broker_->asyncSend(send_message_request, send_message_invocation_context);

  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }
}

class UnitTestSendCallback : public SendCallback {
public:
  UnitTestSendCallback(absl::Mutex& mtx, absl::CondVar& cv, std::string& msg_id, bool& completed)
      : mtx_(mtx), cv_(cv), msg_id_(msg_id), completed_(completed) {}
  void onSuccess(SendResult& send_result) override {
    absl::MutexLock lk(&mtx_);
    msg_id_ = send_result.getMsgId();
    completed_ = true;
    cv_.SignalAll();
  }
  void onException(const MQException& e) override {
    absl::MutexLock lk(&mtx_);
    completed_ = true;
    cv_.SignalAll();
  }

private:
  absl::Mutex& mtx_;
  absl::CondVar& cv_;
  std::string& msg_id_;
  bool& completed_;
};

TEST_F(DefaultMQProducerUnitTest, testAsyncSendMessage) {
  auto producer = std::make_shared<ProducerImpl>(group_name_);
  producer->resourceNamespace(resource_namespace_);
  producer->setNameServerList(name_server_list_);
  producer->setCredentialsProvider(credentials_provider_);
  producer->start();
  MQMessage message;
  message.setTopic(topic_);

  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;
  std::string msg_id;
  auto send_callback = new UnitTestSendCallback(mtx, cv, msg_id, completed);
  producer->send(message, send_callback);
  if (!completed) {
    absl::MutexLock lk(&mtx);
    cv.WaitWithTimeout(&mtx, absl::Seconds(10));
  }
  ASSERT_EQ(msg_id, message_id_);
  producer->shutdown();
}

TEST_F(DefaultMQProducerUnitTest, testSendMessage) {
  auto producer = std::make_shared<ProducerImpl>(group_name_);
  producer->resourceNamespace(resource_namespace_);
  producer->setNameServerList(name_server_list_);
  producer->setCredentialsProvider(credentials_provider_);
  producer->start();
  MQMessage message;
  SendResult send_result = producer->send(message);
  ASSERT_EQ(send_result.getMsgId(), message_id_);
  producer->shutdown();
}

TEST_F(DefaultMQProducerUnitTest, testEndpointIsolation) {
  auto producer = std::make_shared<ProducerImpl>(group_name_);
  producer->resourceNamespace(resource_namespace_);
  producer->setNameServerList(name_server_list_);
  producer->setCredentialsProvider(credentials_provider_);
  producer->start();

  const char* isolated_endpoint = "ipv4:10.0.0.0:10911";
  producer->isolateEndpoint(isolated_endpoint);
  absl::SleepFor(absl::Seconds(10));
  ASSERT_FALSE(producer->isEndpointIsolated(isolated_endpoint));
  producer->shutdown();
}

ROCKETMQ_NAMESPACE_END