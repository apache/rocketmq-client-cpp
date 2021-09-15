#include "TopicPublishInfo.h"
#include "LogInterceptorFactory.h"
#include "RpcClientImpl.h"
#include "Signature.h"
#include "TlsHelper.h"
#include "rocketmq/MQMessageQueue.h"
#include "ClientConfigImpl.h"
#include "gtest/gtest.h"
#include "grpcpp/security/tls_credentials_options.h"

ROCKETMQ_NAMESPACE_BEGIN

class TopicPublishInfoTest : public ::testing::Test {
protected:
  TopicPublishInfoTest() : completion_queue_(std::make_shared<grpc::CompletionQueue>()) {
    server_authorization_check_config_ = std::make_shared<grpc::experimental::TlsServerAuthorizationCheckConfig>(
        std::make_shared<TlsServerAuthorizationChecker>());
    tls_channel_credential_option_.set_server_verification_option(GRPC_TLS_SKIP_HOSTNAME_VERIFICATION);

    std::vector<grpc::experimental::IdentityKeyCertPair> pem_list;
    grpc::experimental::IdentityKeyCertPair pair{};
    pair.private_key = TlsHelper::client_private_key;
    pair.certificate_chain = TlsHelper::client_certificate_chain;
    pem_list.emplace_back(pair);
    certificate_provider_ =
        std::make_shared<grpc::experimental::StaticDataCertificateProvider>(TlsHelper::CA, pem_list);
    tls_channel_credential_option_.set_certificate_provider(certificate_provider_);
    tls_channel_credential_option_.set_server_authorization_check_config(server_authorization_check_config_);
    tls_channel_credential_option_.watch_root_certs();
    tls_channel_credential_option_.watch_identity_key_cert_pairs();
    channel_credential_ = grpc::experimental::TlsCredentials(tls_channel_credential_option_);
    credentials_provider_ = std::make_shared<ConfigFileCredentialsProvider>();
  }

  void SetUp() override {
    std::vector<std::string> name_server_list;

    if (!name_server_list.empty()) {
      target_ = *name_server_list.begin();
    }
    std::vector<std::unique_ptr<grpc::experimental::ClientInterceptorFactoryInterface>> interceptor_factories;
    client_config_.tenantId(tenant_id_);
    client_config_.setCredentialsProvider(credentials_provider_);
    interceptor_factories.emplace_back(absl::make_unique<LogInterceptorFactory>());
    auto channel = grpc::experimental::CreateCustomChannelWithInterceptors(
        target_, channel_credential_, channel_arguments_, std::move(interceptor_factories));
    client_ = std::make_shared<rocketmq::RpcClientImpl>(completion_queue_, channel);

    client_config_.resourceNamespace(resource_namespace_);
    client_config_.setCredentialsProvider(std::make_shared<ConfigFileCredentialsProvider>());
    client_config_.setIoTimeout(absl::Seconds(3));

    Signature::sign(&client_config_, metadata_);
  }

  void TearDown() override { completion_queue_->Shutdown(); }

  std::string topic_{"yc001"};
  std::string group_{"yc001"};
  std::string resource_namespace_{"MQ_INST_1973281269661160_BXmPlOA6"};
  std::string tenant_id_{"sample-tenant"};
  std::string region_id_{"cn-hangzhou"};
  std::string service_name_{"MQ"};
  std::string target_{"dns:grpc.dev:9876"};
  ClientConfigImpl client_config_;
  absl::flat_hash_map<std::string, std::string> metadata_;
  std::shared_ptr<grpc::CompletionQueue> completion_queue_;
  std::shared_ptr<RpcClientImpl> client_;
  CredentialsProviderPtr credentials_provider_;
  std::shared_ptr<grpc::experimental::CertificateProviderInterface> certificate_provider_;
  grpc::experimental::TlsChannelCredentialsOptions tls_channel_credential_option_;
  std::shared_ptr<grpc::experimental::TlsServerAuthorizationCheckConfig> server_authorization_check_config_;
  std::shared_ptr<grpc::ChannelCredentials> channel_credential_;
  grpc::ChannelArguments channel_arguments_;
};

TEST_F(TopicPublishInfoTest, testTopicPublishInfo) {
  rmq::QueryRouteResponse response;
  rmq::QueryRouteRequest request;
  request.mutable_topic()->set_resource_namespace(resource_namespace_);
  request.mutable_topic()->set_name(topic_);
  auto invocation_context = new InvocationContext<QueryRouteResponse>();
  invocation_context->context.set_deadline(std::chrono::system_clock::now() +
                                            absl::ToChronoMilliseconds(client_config_.getIoTimeout()));
  for (const auto& item : metadata_) {
    invocation_context->context.AddMetadata(item.first, item.second);
  }

  auto callback = [this](const InvocationContext<QueryRouteResponse>* invocation_context) {
    if (!invocation_context->status.ok()) {
      std::cout << "code: " << invocation_context->status.error_code()
                << ", message: " << invocation_context->status.error_message() << std::endl;
    } else {
      std::cout << "Response debug string:" << invocation_context->response.DebugString() << std::endl;
    }
    ASSERT_TRUE(invocation_context->status.ok());

    std::vector<Partition> partitions;
    for (const auto& item : invocation_context->response.partitions()) {
      Topic topic(resource_namespace_, topic_);
      Permission permission;
      switch (item.permission()) {
      case rmq::Permission::READ:
        permission = Permission::READ;
        break;
      case rmq::Permission::WRITE:
        permission = Permission::WRITE;
        break;
      case rmq::Permission::READ_WRITE:
        permission = Permission::READ_WRITE;
        break;
      default:
        permission = Permission::NONE;
        break;
      }

      AddressScheme scheme;
      switch (item.broker().endpoints().scheme()) {
      case rmq::AddressScheme::IPv4:
        scheme = AddressScheme::IPv4;
        break;
      case rmq::AddressScheme::IPv6:
        scheme = AddressScheme::IPv6;
        break;
      case rmq::AddressScheme::DOMAIN_NAME:
        scheme = AddressScheme::DOMAIN_NAME;
        break;
      default:
        scheme = AddressScheme::IPv4;
      }

      std::vector<Address> addresses;
      for (const auto& host_port : item.broker().endpoints().addresses()) {
        Address address(host_port.host(), host_port.port());
        addresses.emplace_back(address);
      }
      ServiceAddress service_address(scheme, addresses);

      Broker broker(item.broker().name(), item.broker().id(), service_address);
      Partition partition(topic, item.id(), permission, broker);
      partitions.emplace_back(partition);
    }
    auto topic_route_data =
        std::make_shared<TopicRouteData>(partitions, invocation_context->response.common().DebugString());
    rocketmq::TopicPublishInfo topic_publish_info(topic_, topic_route_data);
    rocketmq::MQMessageQueue message_queue;
    EXPECT_TRUE(topic_publish_info.selectOneMessageQueue(message_queue));

    EXPECT_STREQ(topic_.c_str(), message_queue.getTopic().c_str());
  };

  invocation_context->callback = callback;

  client_->asyncQueryRoute(request, invocation_context);
}

ROCKETMQ_NAMESPACE_END
