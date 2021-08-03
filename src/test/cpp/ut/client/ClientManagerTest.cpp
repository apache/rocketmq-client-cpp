#include "ClientManagerImpl.h"
#include "ReceiveMessageCallbackMock.h"
#include "RpcClientMock.h"
#include "apache/rocketmq/v1/definition.pb.h"
#include "gtest/gtest.h"
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
    receive_message_callback_ = std::make_shared<testing::NiceMock<ReceiveMessageCallbackMock>>();
    metadata_.insert({"foo", "bar"});
    metadata_.insert({"name", "Donald.J.Trump"});
  }

  void TearDown() override { client_manager_->shutdown(); }

protected:
  std::string arn_{"arn:mq://test"};
  std::string topic_{"TestTopic"};
  std::string target_host_{"ipv4:10.0.0.0:10911"};
  std::shared_ptr<ClientManagerImpl> client_manager_;
  std::shared_ptr<testing::NiceMock<RpcClientMock>> rpc_client_;
  std::shared_ptr<testing::NiceMock<ReceiveMessageCallbackMock>> receive_message_callback_;
  absl::Duration io_timeout_{absl::Seconds(3)};
  Metadata metadata_;
  std::string message_body_{"Message body"};
  std::string tag_{"TagA"};
  std::string key_{"key-0"};
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

  QueryRouteRequest request;
  request.mutable_topic()->set_arn(arn_);
  request.mutable_topic()->set_name(topic_);
  auto callback = [&](bool, const TopicRouteDataPtr&) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
  };
  client_manager_->resolveRoute(target_host_, metadata_, request, absl::ToChronoMilliseconds(io_timeout_), callback);
  {
    absl::MutexLock lk(&mtx);
    cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
  }
  EXPECT_TRUE(completed);
}

TEST_F(ClientManagerTest, testQueryAssignment) {

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  auto mock_query_assignment = [&](const QueryAssignmentRequest& request,
                                   InvocationContext<QueryAssignmentResponse>* invocation_context) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
    invocation_context->onCompletion(true);
  };

  EXPECT_CALL(*rpc_client_, asyncQueryAssignment)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_query_assignment));
  QueryAssignmentRequest request;
  bool callback_invoked = false;
  auto callback = [&](bool ok, const QueryAssignmentResponse& response) { callback_invoked = true; };

  client_manager_->queryAssignment(target_host_, metadata_, request, absl::ToChronoMilliseconds(io_timeout_), callback);

  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);
  EXPECT_TRUE(callback_invoked);
}

TEST_F(ClientManagerTest, testReceiveMessage) {

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  auto mock_async_receive = [&](const ReceiveMessageRequest& request,
                                InvocationContext<ReceiveMessageResponse>* invocation_context) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
    invocation_context->onCompletion(true);
  };

  EXPECT_CALL(*rpc_client_, asyncReceive)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_async_receive));
  ReceiveMessageRequest request;

  EXPECT_CALL(*receive_message_callback_, onSuccess).Times(testing::AtLeast(1));

  client_manager_->receiveMessage(target_host_, metadata_, request, absl::ToChronoMilliseconds(io_timeout_),
                                  receive_message_callback_);

  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);
}

TEST_F(ClientManagerTest, testReceiveMessage_Failure) {

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  auto mock_async_receive = [&](const ReceiveMessageRequest& request,
                                InvocationContext<ReceiveMessageResponse>* invocation_context) {
    invocation_context->status = grpc::Status::CANCELLED;
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
    invocation_context->onCompletion(true);
  };

  EXPECT_CALL(*rpc_client_, asyncReceive)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_async_receive));
  ReceiveMessageRequest request;

  EXPECT_CALL(*receive_message_callback_, onException).Times(testing::AtLeast(1));

  client_manager_->receiveMessage(target_host_, metadata_, request, absl::ToChronoMilliseconds(io_timeout_),
                                  receive_message_callback_);

  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);
}

TEST_F(ClientManagerTest, testAck) {
  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  auto mock_ack = [&](const AckMessageRequest& request, InvocationContext<AckMessageResponse>* invocation_context) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
    invocation_context->onCompletion(true);
  };

  EXPECT_CALL(*rpc_client_, asyncAck).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(mock_ack));
  AckMessageRequest request;
  bool callback_invoked = false;
  auto callback = [&](bool ok) { callback_invoked = true; };

  client_manager_->ack(target_host_, metadata_, request, absl::ToChronoMilliseconds(io_timeout_), callback);

  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);
  EXPECT_TRUE(callback_invoked);
}

TEST_F(ClientManagerTest, testNack) {
  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  auto mock_nack = [&](const NackMessageRequest& request, InvocationContext<NackMessageResponse>* invocation_context) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
    invocation_context->onCompletion(true);
  };

  EXPECT_CALL(*rpc_client_, asyncNack).Times(testing::AtLeast(1)).WillRepeatedly(testing::Invoke(mock_nack));
  NackMessageRequest request;
  bool callback_invoked = false;
  auto callback = [&](bool ok) { callback_invoked = true; };

  client_manager_->nack(target_host_, metadata_, request, absl::ToChronoMilliseconds(io_timeout_), callback);

  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);
  EXPECT_TRUE(callback_invoked);
}

TEST_F(ClientManagerTest, testForwardMessageToDeadLetterQueue) {
  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  auto mock_forward = [&](const ForwardMessageToDeadLetterQueueRequest& request,
                          InvocationContext<ForwardMessageToDeadLetterQueueResponse>* invocation_context) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
    invocation_context->onCompletion(true);
  };

  EXPECT_CALL(*rpc_client_, asyncForwardMessageToDeadLetterQueue)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_forward));
  ForwardMessageToDeadLetterQueueRequest request;
  bool callback_invoked = false;
  auto callback = [&](bool ok) { callback_invoked = true; };

  client_manager_->forwardMessageToDeadLetterQueue(target_host_, metadata_, request,
                                                   absl::ToChronoMilliseconds(io_timeout_), callback);
  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);
  EXPECT_TRUE(callback_invoked);
}

TEST_F(ClientManagerTest, testMultiplexingCall) {}

TEST_F(ClientManagerTest, testEndTransaction) {
  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  auto mock_end_transaction = [&](const EndTransactionRequest& request,
                                  InvocationContext<EndTransactionResponse>* invocation_context) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
    invocation_context->onCompletion(true);
  };

  EXPECT_CALL(*rpc_client_, asyncEndTransaction)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_end_transaction));
  EndTransactionRequest request;
  bool callback_invoked = false;
  auto callback = [&](bool ok, const EndTransactionResponse& response) { callback_invoked = true; };

  client_manager_->endTransaction(target_host_, metadata_, request, absl::ToChronoMilliseconds(io_timeout_), callback);
  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);
  EXPECT_TRUE(callback_invoked);
}

TEST_F(ClientManagerTest, processPullResult) {
  grpc::ClientContext client_context;
  PullMessageResponse response;
  auto message = new rmq::Message;
  message->mutable_topic()->set_arn(arn_);
  message->mutable_topic()->set_name(topic_);
  message->set_body(message_body_);
  message->mutable_system_attribute()->mutable_body_digest()->set_type(rmq::DigestType::MD5);
  std::string checksum;
  EXPECT_TRUE(MixAll::md5(message_body_, checksum));
  message->mutable_system_attribute()->mutable_body_digest()->set_checksum(checksum);
  message->mutable_system_attribute()->set_tag(tag_);
  message->mutable_system_attribute()->mutable_keys()->Add(key_.c_str());
  response.mutable_messages()->AddAllocated(message);
  ReceiveMessageResult result;
  client_manager_->processPullResult(client_context, response, result, target_host_);
  EXPECT_EQ(1, result.messages_.size());
  const auto& msg = *result.messages_.begin();
  EXPECT_EQ(msg.getTopic(), topic_);
  EXPECT_EQ(msg.getTags(), tag_);
  EXPECT_EQ(msg.getBody(), message_body_);
}

TEST_F(ClientManagerTest, testHealthCheck) {
  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;

  auto mock_health_check = [&](const HealthCheckRequest& request,
                               InvocationContext<HealthCheckResponse>* invocation_context) {
    absl::MutexLock lk(&mtx);
    completed = true;
    cv.SignalAll();
    invocation_context->onCompletion(true);
  };

  EXPECT_CALL(*rpc_client_, asyncHealthCheck)
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::Invoke(mock_health_check));
  HealthCheckRequest request;
  bool callback_invoked = false;
  auto callback = [&](const std::string& target_host,
                      const InvocationContext<HealthCheckResponse>* invocation_context) { callback_invoked = true; };

  client_manager_->healthCheck(target_host_, metadata_, request, absl::ToChronoMilliseconds(io_timeout_), callback);
  {
    absl::MutexLock lk(&mtx);
    if (!completed) {
      cv.WaitWithDeadline(&mtx, absl::Now() + absl::Seconds(3));
    }
  }
  EXPECT_TRUE(completed);
  EXPECT_TRUE(callback_invoked);
}

ROCKETMQ_NAMESPACE_END