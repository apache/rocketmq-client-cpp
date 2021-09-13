#include "ClientConfigImpl.h"
#include "InvocationContext.h"
#include "LogInterceptorFactory.h"
#include "MixAll.h"
#include "RpcClientImpl.h"
#include "Signature.h"
#include "TlsHelper.h"
#include "UniqueIdGenerator.h"
#include "UtilAll.h"
#include "absl/container/flat_hash_set.h"
#include "apache/rocketmq/v1/service.pb.h"
#include "rocketmq/CredentialsProvider.h"
#include "rocketmq/Logger.h"
#include "spdlog/spdlog.h"
#include "gtest/gtest.h"
#include <thread>
#include <unordered_map>
#include <iostream>

using namespace testing;

ROCKETMQ_NAMESPACE_BEGIN

class RpcClientTest : public ::testing::Test {
protected:
  RpcClientTest() : completion_queue_(std::make_shared<grpc::CompletionQueue>()) {

    server_authorization_check_config_ = std::make_shared<grpc::experimental::TlsServerAuthorizationCheckConfig>(
        std::make_shared<TlsServerAuthorizationChecker>());
    std::vector<grpc::experimental::IdentityKeyCertPair> pem_list;
    grpc::experimental::IdentityKeyCertPair pair{.private_key = TlsHelper::client_private_key,
                                                 .certificate_chain = TlsHelper::client_certificate_chain};
    pem_list.emplace_back(pair);
    certificate_provider_ =
        std::make_shared<grpc::experimental::StaticDataCertificateProvider>(TlsHelper::CA, pem_list);
    tls_channel_credential_options_.set_certificate_provider(certificate_provider_);
    tls_channel_credential_options_.set_server_verification_option(GRPC_TLS_SKIP_ALL_SERVER_VERIFICATION);
    tls_channel_credential_options_.set_server_authorization_check_config(server_authorization_check_config_);
    tls_channel_credential_options_.watch_root_certs();
    tls_channel_credential_options_.watch_identity_key_cert_pairs();
    channel_credential_ = grpc::experimental::TlsCredentials(tls_channel_credential_options_);
    credentials_provider_ = std::make_shared<ConfigFileCredentialsProvider>();
    client_config_.tenantId(tenant_id_);
    client_config_.setCredentialsProvider(credentials_provider_);
  }

  void SetUp() override {
    getLogger().setLevel(Level::Debug);
    client_config_.setCredentialsProvider(std::make_shared<ConfigFileCredentialsProvider>());
    client_config_.resourceNamespace(resource_namespace_);
    client_config_.region(region_id_);
    client_config_.tenantId(tenant_id_);

    Signature::sign(&client_config_, metadata_);

    std::vector<std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>> interceptor_factories;
    interceptor_factories.emplace_back(absl::make_unique<LogInterceptorFactory>());
    name_server_channel_ = grpc::experimental::CreateCustomChannelWithInterceptors(
        name_server_target_, channel_credential_, channel_arguments_, std::move(interceptor_factories));
  }

  void TearDown() override {}

  ~RpcClientTest() override { completion_queue_->Shutdown(); }

  bool brokerEndpoint(const std::string& topic, std::string& endpoint) {
    QueryRouteRequest route_request;
    route_request.mutable_topic()->set_name(topic.data());
    QueryRouteResponse route_response;
    RpcClientImpl name_server_client(completion_queue_, name_server_channel_);

    absl::Mutex mtx;
    absl::CondVar cv;
    bool completed = false;

    auto callback = [&](const InvocationContext<QueryRouteResponse>* invocation_context) {
      ASSERT_TRUE(invocation_context->status.ok());
      std::cout << "Route Response:" << route_response.DebugString() << std::endl;
      absl::MutexLock lk(&mtx);
      route_response = invocation_context->response;
      completed = true;
      cv.SignalAll();
    };

    auto invocation_context = new InvocationContext<QueryRouteResponse>();
    invocation_context->callback = callback;
    invocation_context->context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(3));

    for (const auto& item : metadata_) {
      invocation_context->context.AddMetadata(item.first, item.second);
    }

    name_server_client.asyncQueryRoute(route_request, invocation_context);

    absl::flat_hash_set<std::string> broker_addresses;
    for (auto& partition : route_response.partitions()) {
      auto& broker = partition.broker();
      if (MixAll::MASTER_BROKER_ID == broker.id()) {
        for (auto& item : broker.endpoints().addresses()) {
          std::string connect_string(fmt::format("{}:{}", item.host(), item.port()));
          if (!broker_addresses.contains(connect_string)) {
            broker_addresses.insert(connect_string);
          }
        }
      } else {
        for (const auto& item : broker.endpoints().addresses()) {
          SPDLOG_WARN("Unexpected endpoint[{}:{}] with brokerId={}", item.host(), item.port(), broker.id());
        }
      }
    }
    if (broker_addresses.empty()) {
      return false;
    }

    endpoint = *broker_addresses.begin();
    return true;
  }

  void fillSendMessageRequest(SendMessageRequest& send_message_request) {
    send_message_request.mutable_message()->mutable_topic()->set_resource_namespace(resource_namespace_);
    send_message_request.mutable_message()->mutable_topic()->set_name(topic_);
    std::unordered_map<std::string, std::string> props;
    props["key"] = "value";
    props["Jack"] = "Bauer";
    send_message_request.mutable_message()->mutable_user_attribute()->insert(props.begin(), props.end());
    auto system_attribute = send_message_request.mutable_message()->mutable_system_attribute();
    system_attribute->set_message_id(message_id_);
    system_attribute->set_message_type(rmq::MessageType::NORMAL);
    system_attribute->set_body_encoding(rmq::Encoding::IDENTITY);
    system_attribute->set_born_host(UtilAll::hostname());
    system_attribute->set_tag("TagA");
    send_message_request.mutable_message()->set_body("Example data");
  }

  RpcClientSharedPtr brokerRpcClient() {
    std::string broker_endpoint;
    bool success = brokerEndpoint(topic_, broker_endpoint);
    if (!success) {
      return nullptr;
    }
    SPDLOG_INFO("Target broker address: {}", broker_endpoint);

    std::vector<std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>> interceptor_factories;
    interceptor_factories.emplace_back(absl::make_unique<LogInterceptorFactory>());
    auto broker_channel = grpc::experimental::CreateCustomChannelWithInterceptors(
        broker_endpoint, channel_credential_, channel_arguments_, std::move(interceptor_factories));
    auto client = std::make_shared<RpcClientImpl>(completion_queue_, broker_channel);
    return client;
  }

  void fillHeartbeatRequest(HeartbeatRequest& heartbeat_request) {
    heartbeat_request.set_client_id("client_id_0");
    auto consumer_data = heartbeat_request.mutable_consumer_data();
    consumer_data->mutable_group()->set_resource_namespace(resource_namespace_);
    consumer_data->mutable_group()->set_name(topic_);

    auto subscription_entry = new rmq::SubscriptionEntry;
    subscription_entry->mutable_topic()->set_name(topic_);
    subscription_entry->mutable_topic()->set_resource_namespace(resource_namespace_);
    subscription_entry->mutable_expression()->set_type(rmq::FilterType::TAG);
    subscription_entry->mutable_expression()->set_expression("*");
    consumer_data->mutable_subscriptions()->AddAllocated(subscription_entry);
    heartbeat_request.set_fifo_flag(false);
  }

  std::string name_server_target_{"47.98.116.189:80"};
  std::shared_ptr<grpc::CompletionQueue> completion_queue_;
  std::shared_ptr<grpc::Channel> name_server_channel_;
  std::string topic_{"cpp_sdk_standard"};
  std::string group_{"GID_cpp_sdk_standard"};
  std::string resource_namespace_{"MQ_INST_1080056302921134_BXuIbML7"};
  std::string tenant_id_{"sample-tenant"};
  std::string region_id_{"cn-hangzhou"};
  std::string service_name_{"MQ"};
  absl::flat_hash_map<std::string, std::string> metadata_;
  ClientConfigImpl client_config_;
  CredentialsProviderPtr credentials_provider_;
  std::shared_ptr<grpc::experimental::StaticDataCertificateProvider> certificate_provider_;
  grpc::experimental::TlsChannelCredentialsOptions tls_channel_credential_options_;
  std::shared_ptr<grpc::experimental::TlsServerAuthorizationCheckConfig> server_authorization_check_config_;
  std::shared_ptr<grpc::ChannelCredentials> channel_credential_;
  grpc::ChannelArguments channel_arguments_;
  std::string message_id_{UniqueIdGenerator::instance().next()};
};

TEST_F(RpcClientTest, testRouteInfo) {
  RpcClientImpl client(completion_queue_, name_server_channel_);
  QueryRouteRequest request;
  request.mutable_topic()->set_name(topic_);
  request.mutable_topic()->set_resource_namespace(resource_namespace_);
  auto invocation_context = new InvocationContext<QueryRouteResponse>();
  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;
  auto callback = [&](const InvocationContext<QueryRouteResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      SPDLOG_ERROR("Status not OK");
    }
    ASSERT_TRUE(invocation_context->status.ok());
    EXPECT_TRUE(google::rpc::Code::OK == invocation_context->response.common().status().code());
    EXPECT_FALSE(invocation_context->response.partitions().empty());
    {
      absl::MutexLock lk(&mtx);
      cv.SignalAll();
    }
  };

  invocation_context->callback = callback;
  invocation_context->context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(3));
  for (const auto& item : metadata_) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }
  client.asyncQueryRoute(request, invocation_context);

  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }
}

TEST_F(RpcClientTest, DISABLED_testSendMessageAsync) {
  SendMessageRequest request;
  fillSendMessageRequest(request);
  auto client = brokerRpcClient();
  auto context = new InvocationContext<SendMessageResponse>();

  for (const auto& entry : metadata_) {
    context->context.AddMetadata(entry.first, entry.second);
  }

  context->callback = [](const InvocationContext<SendMessageResponse>* invocation_context) {
    if ((!invocation_context->status.ok())) {
      std::cout << "error code: " << invocation_context->status.error_code()
                << ", error message: " << invocation_context->status.error_message() << std::endl;
    }
    ASSERT_TRUE(invocation_context->status.ok());
  };
  client->asyncSend(request, context);
  std::thread th([&]() {
    InvocationContext<SendMessageResponse>* ctx;
    bool ok = false;
    completion_queue_->Next(reinterpret_cast<void**>(&ctx), &ok);
    if (ok) {
      ctx->onCompletion(ok);
    }
  });

  if (th.joinable()) {
    th.join();
  }
}

TEST_F(RpcClientTest, DISABLED_testHeartbeat) {
  auto client = brokerRpcClient();
  HeartbeatRequest heartbeat_request;
  HeartbeatResponse response;
  fillHeartbeatRequest(heartbeat_request);
  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;
  auto invocation_context = new InvocationContext<HeartbeatResponse>();

  for (const auto& entry : metadata_) {
    invocation_context->context.AddMetadata(entry.first, entry.second);
  }

  invocation_context->callback = [&](const InvocationContext<HeartbeatResponse>* invocation_context) {
    ASSERT_TRUE(invocation_context->status.ok());
    EXPECT_TRUE(google::rpc::Code::OK == invocation_context->response.common().status().code());
    {
      completed = true;
      absl::MutexLock lk(&mtx);
      cv.SignalAll();
    }
  };

  client->asyncHeartbeat(heartbeat_request, invocation_context);

  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }
}

TEST_F(RpcClientTest, DISABLED_testQueryAssignment) {
  auto client = brokerRpcClient();
  QueryAssignmentRequest request;
  request.mutable_topic()->set_name(topic_);
  request.mutable_topic()->set_resource_namespace(resource_namespace_);
  request.mutable_group()->set_resource_namespace(resource_namespace_);
  request.mutable_group()->set_name(group_);
  QueryAssignmentResponse response;

  auto invocation_context = new InvocationContext<QueryAssignmentResponse>();

  for (const auto& entry : metadata_) {
    invocation_context->context.AddMetadata(entry.first, entry.second);
  }

  bool completed = false;
  absl::Mutex mtx;
  absl::CondVar cv;
  auto callback = [&](const InvocationContext<QueryAssignmentResponse>* invocation_context) {
    ASSERT_TRUE(invocation_context->status.ok());
    ASSERT_FALSE(invocation_context->response.assignments().empty());
    completed = true;
    {
      absl::MutexLock lk(&mtx);
      cv.SignalAll();
    }
  };
  invocation_context->callback = callback;
  client->asyncQueryAssignment(request, invocation_context);
  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }
}

TEST_F(RpcClientTest, DISABLED_testHealthCheck) {
  auto client = brokerRpcClient();
  HealthCheckRequest request;
  const std::string& client_host = UtilAll::hostname();
  request.set_client_host(client_host);
  HealthCheckResponse response;
  auto invocation_context = new InvocationContext<HealthCheckResponse>();

  for (const auto& entry : metadata_) {
    invocation_context->context.AddMetadata(entry.first, entry.second);
  }

  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;
  invocation_context->callback = [&](const InvocationContext<HealthCheckResponse>* invocation_context) {
    {
      absl::MutexLock lk(&mtx);
      completed = true;
      cv.SignalAll();
    }
    EXPECT_TRUE(invocation_context->status.ok());
    EXPECT_EQ(google::rpc::Code::OK, invocation_context->response.common().status().code());
  };
  client->asyncHealthCheck(request, invocation_context);
  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }
}

ROCKETMQ_NAMESPACE_END