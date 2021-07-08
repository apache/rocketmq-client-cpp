#include "rocketmq/DefaultMQProducer.h"

#include "DefaultMQProducerImpl.h"
#include "MQClientTest.h"
#include "rocketmq/MQSelector.h"
#include <mutex>
#include <utility>

ROCKETMQ_NAMESPACE_BEGIN

class DefaultMQProducerUnitTest : public MQClientTest {
public:
  DefaultMQProducerUnitTest() : MQClientTest() {
    // More set-up
    rpc_client_broker_ = std::make_shared<testing::NiceMock<RpcClientMock>>();

    ON_CALL(*rpc_client_broker_, needHeartbeat).WillByDefault(testing::Return(true));

    ON_CALL(*rpc_client_broker_, ok()).WillByDefault(testing::Invoke([this]() { return client_ok_; }));

    ON_CALL(*rpc_client_broker_, asyncHealthCheck)
        .WillByDefault(testing::Invoke(
            [this](const HealthCheckRequest& request, InvocationContext<HealthCheckResponse>* invocation_context) {
              invocation_context->response_.mutable_common()->mutable_status()->set_code(ok_);
              invocation_context->onCompletion(true);
            }));

    ON_CALL(*rpc_client_broker_, asyncHeartbeat(testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            [this](const HeartbeatRequest& request, InvocationContext<HeartbeatResponse>* invocation_context) {
              invocation_context->response_.mutable_common()->mutable_status()->set_code(ok_);
              invocation_context->onCompletion(true);
            }));

    ON_CALL(*rpc_client_broker_, asyncSend)
        .WillByDefault(testing::Invoke(
            [this](const SendMessageRequest& request, InvocationContext<SendMessageResponse>* invocation_context) {
              invocation_context->response_.mutable_common()->mutable_status()->set_code(ok_);
              invocation_context->response_.set_message_id(message_id_);
              invocation_context->onCompletion(true);
            }));

    const char* address_format = "ipv4:10.0.0.{}:10911";
    for (int i = 0; i < partition_num_ / avg_partition_per_host_; ++i) {
      std::string&& address = fmt::format(address_format, i);
      client_instance_->addRpcClient(address, rpc_client_broker_);
    }
  }

protected:
  absl::flat_hash_map<std::string, std::string> metadata_;
  std::shared_ptr<testing::NiceMock<RpcClientMock>> rpc_client_broker_;
  std::string message_id_{"msg_id_0"};
};

TEST_F(DefaultMQProducerUnitTest, testBasicSetUp) {
  QueryRouteRequest request;
  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;
  auto invocation_context = new InvocationContext<QueryRouteResponse>();
  auto callback = [&](const grpc::Status& status, const grpc::ClientContext& context,
                      const QueryRouteResponse& response) {
    ASSERT_TRUE(status.ok());
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
  };
  invocation_context->callback_ = callback;
  rpc_client_ns_->asyncQueryRoute(request, invocation_context);
  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }

  completed = false;
  SendMessageRequest send_message_request;
  auto send_message_invocation_context = new InvocationContext<SendMessageResponse>();
  auto send_callback = [&](const grpc::Status& status, const grpc::ClientContext& context,
                           const SendMessageResponse& response) {
    ASSERT_TRUE(response.message_id() == message_id_);
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
  };
  send_message_invocation_context->callback_ = send_callback;

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
  void onSuccess(const SendResult& send_result) override {
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
  auto producer = std::make_shared<DefaultMQProducerImpl>(group_name_);
  producer->arn(arn_);
  producer->setNameServerList(name_server_list_);
  producer->start();
  MQMessage message;
  message.setTopic(topic_);

  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;
  std::string msg_id;
  auto send_callback = new UnitTestSendCallback(mtx, cv, msg_id, completed);
  producer->send(message, send_callback);
  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }
  ASSERT_EQ(msg_id, message_id_);
}

TEST_F(DefaultMQProducerUnitTest, testSendMessage) {
  auto producer = std::make_shared<DefaultMQProducerImpl>(group_name_);
  producer->arn(arn_);
  producer->setNameServerList(name_server_list_);
  producer->start();
  MQMessage message;
  SendResult send_result = producer->send(message);
  ASSERT_EQ(send_result.getMsgId(), message_id_);
}

ROCKETMQ_NAMESPACE_END