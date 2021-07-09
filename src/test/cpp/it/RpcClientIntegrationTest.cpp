#include <iostream>
#include <thread>
#include <unordered_map>

#include "ClientConfig.h"
#include "MixAll.h"
#include "Protocol.h"
#include "RpcClientImpl.h"
#include "absl/strings/string_view.h"
#include "gtest/gtest.h"

using namespace testing;

ROCKETMQ_NAMESPACE_BEGIN

class RpcClientIntegrationTest : public testing::Test {
protected:
  RpcClientIntegrationTest()
      : topic_name("TestTopic"), group_name("CID_sample"), client_id("client_id_01"), tag_name("TagA"),
        completion_queue_(std::make_shared<grpc::CompletionQueue>()),
        rpcClient(completion_queue_, grpc::CreateChannel(target, grpc::InsecureChannelCredentials())) {}

  void SetUp() override {
    {
      heartbeat_request.mutable_heartbeats()->Clear();

      auto heartbeat_entry = new rmq::HeartbeatEntry;
      heartbeat_entry->set_client_id(client_id);
      heartbeat_entry->mutable_consumer_group()->mutable_group()->set_name(group_name);
      auto subscription_entry = new rmq::SubscriptionEntry;
      subscription_entry->mutable_topic()->set_name(topic_name);
      subscription_entry->mutable_expression()->set_type(rmq::FilterType::TAG);
      subscription_entry->mutable_expression()->set_expression(tag_name);
      heartbeat_entry->mutable_consumer_group()->mutable_subscriptions()->AddAllocated(subscription_entry);
    }

    {
      query_assignment_request.mutable_topic()->set_name(topic_name);
      query_assignment_request.set_client_id(client_id);
      query_assignment_request.mutable_group()->set_name(group_name);
    }

    {
      receive_message_request.mutable_group()->set_name(group_name);
      receive_message_request.set_client_id(client_id);
      receive_message_request.mutable_partition()->set_id(0);
      receive_message_request.mutable_partition()->mutable_topic()->set_name(topic_name);
      receive_message_request.mutable_partition()->mutable_broker()->set_name("broker-a");
      auto endpoint = receive_message_request.mutable_partition()->mutable_broker()->mutable_endpoints();
      endpoint->set_scheme(rmq::AddressScheme::IPv4);
      auto address = new rmq::Address;
      address->set_host("127.0.0.1");
      address->set_port(10911);
      endpoint->mutable_addresses()->AddAllocated(address);
      receive_message_request.set_batch_size(32);
      receive_message_request.mutable_filter_expression()->set_type(rmq::FilterType::TAG);
      receive_message_request.mutable_filter_expression()->set_expression(tag_name);
      receive_message_request.set_consume_policy(rmq::ConsumePolicy::RESUME);
      receive_message_request.mutable_invisible_duration()->set_seconds(30);
      receive_message_request.mutable_await_time()->set_seconds(10);
    }
  }

  std::shared_ptr<grpc::CompletionQueue> completion_queue_;
  std::string target{"127.0.0.1:10921"};
  std::string topic_name;
  std::string group_name;
  std::string client_id;
  std::string tag_name;
  RpcClientImpl rpcClient;
  rmq::HeartbeatRequest heartbeat_request;
  rmq::QueryAssignmentRequest query_assignment_request;
  rmq::ReceiveMessageRequest receive_message_request;
};

TEST_F(RpcClientIntegrationTest, testQueryAssignment_without_heartbeat) {
  QueryAssignmentResponse response;
  auto status = rpcClient.queryAssignment(query_assignment_request, &response);
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(response.common().status().code(), google::rpc::Code::OK);
  EXPECT_EQ(8, response.assignments().size());
  for (auto& assignment : response.assignments()) {
    auto& partition = assignment.partition();
    std::cout << partition.topic().name() << " --> " << partition.broker().name() << " --> " << partition.id()
              << std::endl;
  }
}

TEST_F(RpcClientIntegrationTest, testQueryAssignment_with_prior_heartbeat) {

  {
    HeartbeatResponse response;
    grpc::ClientContext client_context;
    auto status = rpcClient.heartbeat(client_context, heartbeat_request, &response);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(google::rpc::Code::OK, response.common().status().code());
  }

  QueryAssignmentResponse response;

  auto status = rpcClient.queryAssignment(query_assignment_request, &response);
  EXPECT_TRUE(status.ok());
  EXPECT_EQ(google::rpc::Code::OK, response.common().status().code());
  EXPECT_FALSE(response.assignments().empty());
}

TEST_F(RpcClientIntegrationTest, testQueryAssignment_with_mutiple_clients) {
  std::string consumer_group("CID_sample");
  std::string client_id_0("CID_sample_member_0");
  std::string client_id_1("CID_sample_member_1");
  std::string topic("TopicTest");

  {
    // Send heartbeat to broker
    HeartbeatResponse response;
    grpc::ClientContext client_context;
    auto status = rpcClient.heartbeat(client_context, heartbeat_request, &response);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(google::rpc::Code::OK, response.common().status().code());
  }

  {
    HeartbeatResponse response;
    grpc::ClientContext client_context;
    auto status = rpcClient.heartbeat(client_context, heartbeat_request, &response);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(google::rpc::Code::OK, response.common().status().code());
  }

  {
    QueryAssignmentResponse response;

    auto status = rpcClient.queryAssignment(query_assignment_request, &response);
    EXPECT_TRUE(status.ok());
    EXPECT_EQ(google::rpc::Code::OK, response.common().status().code());
    EXPECT_FALSE(response.assignments().empty());
  }
}

TEST_F(RpcClientIntegrationTest, testPop) {

  auto invocation_context = new InvocationContext<ReceiveMessageResponse>;

  auto callback = [](const grpc::Status& status, const grpc::ClientContext& client_context,
                     const ReceiveMessageResponse& response) {
    for (int i = 0; i < response.messages_size(); ++i) {
      auto msg = response.messages().at(i);
      std::cout << msg.system_attribute().message_id() << std::endl;
      const std::string& receipt_handle = msg.system_attribute().receipt_handle();
      EXPECT_FALSE(receipt_handle.empty());
    }
  };

  invocation_context->callback = callback;
  auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
  invocation_context->context.set_deadline(deadline);

  rpcClient.asyncReceive(receive_message_request, invocation_context);
  void* tag;
  bool ok;
  bool stop = false;
  while (!stop) {
    const auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(10);
    grpc::CompletionQueue::NextStatus status = rpcClient.completionQueue()->AsyncNext(&tag, &ok, deadline);
    EXPECT_FALSE(grpc::CompletionQueue::SHUTDOWN == status);
    switch (status) {
    case grpc::CompletionQueue::SHUTDOWN:
      stop = true;
      break;
    case grpc::CompletionQueue::GOT_EVENT: {
      stop = true;
      auto invocation_ctx = reinterpret_cast<InvocationContext<ReceiveMessageResponse>*>(tag);
      EXPECT_TRUE(nullptr != invocation_ctx);
      invocation_ctx->onCompletion(ok);
      break;
    }
    case grpc::CompletionQueue::TIMEOUT:
      break;
    }
  }
}

TEST_F(RpcClientIntegrationTest, testPopAndAck) {
  auto invocation_context = new InvocationContext<ReceiveMessageResponse>;

  auto callback = [&](const grpc::Status& status, const grpc::ClientContext& client_context,
                      const ReceiveMessageResponse& response) {
    for (int i = 0; i < response.messages_size(); ++i) {
      auto msg = response.messages().at(i);
      const std::string& message_id = msg.system_attribute().message_id();
      const std::string& receipt_handle = msg.system_attribute().receipt_handle();

      EXPECT_FALSE(message_id.empty());
      EXPECT_FALSE(receipt_handle.empty());

      AckMessageRequest ack_request;
      ack_request.set_client_id(client_id);
      ack_request.mutable_group()->set_name(group_name);
      ack_request.mutable_topic()->set_name(topic_name);
      ack_request.set_message_id(message_id);
      ack_request.set_receipt_handle(receipt_handle);
      InvocationContext<AckMessageResponse> invocation_context;

      std::mutex ack_mtx;
      std::condition_variable ack_cv;
      grpc::Status ack_rpc_status = grpc::Status::CANCELLED;
      google::rpc::Status ack_status;
      std::atomic_bool ack_done(false);

      invocation_context.callback = [&](const grpc::Status& status, const grpc::ClientContext& client_context,
                                         const AckMessageResponse& response) {
        ack_rpc_status = status;
        ack_status = response.common().status();
        ack_done.store(true);
        {
          std::unique_lock<std::mutex> lk(ack_mtx);
          ack_cv.notify_all();
        }
      };
      rpcClient.asyncAck(ack_request, &invocation_context);

      // Wait for Ack completion.
      {
        std::unique_lock<std::mutex> lk(ack_mtx);
        ack_cv.wait(lk, [&]() { return ack_done.load(); });
      }

      // Verify gRPC is OK
      EXPECT_TRUE(ack_rpc_status.ok());
      EXPECT_EQ(google::rpc::Code::OK, ack_status.code());
    }
  };

  invocation_context->callback = callback;
  auto deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
  invocation_context->context.set_deadline(deadline);

  rpcClient.asyncReceive(receive_message_request, invocation_context);
  void* tag;
  bool ok;
  bool stop = false;
  while (!stop) {
    const auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(10);
    grpc::CompletionQueue::NextStatus status = rpcClient.completionQueue()->AsyncNext(&tag, &ok, deadline);
    EXPECT_FALSE(grpc::CompletionQueue::SHUTDOWN == status);
    switch (status) {
    case grpc::CompletionQueue::SHUTDOWN:
      stop = true;
      break;
    case grpc::CompletionQueue::GOT_EVENT: {
      stop = true;
      auto invocation_ctx = reinterpret_cast<InvocationContext<ReceiveMessageResponse>*>(tag);
      EXPECT_TRUE(nullptr != invocation_ctx);
      invocation_ctx->onCompletion(ok);
      break;
    }
    case grpc::CompletionQueue::TIMEOUT:
      break;
    }
  }
}

ROCKETMQ_NAMESPACE_END