#include "rocketmq/DefaultMQPullConsumer.h"

#include "absl/strings/str_split.h"

#include "AwaitPullCallback.h"
#include "DynamicNameServerResolver.h"
#include "PullConsumerImpl.h"
#include "StaticNameServerResolver.h"
#include <memory>

ROCKETMQ_NAMESPACE_BEGIN

DefaultMQPullConsumer::DefaultMQPullConsumer(const std::string& group_name)
    : impl_(std::make_shared<PullConsumerImpl>(group_name)) {}

void DefaultMQPullConsumer::start() { impl_->start(); }

void DefaultMQPullConsumer::shutdown() { impl_->shutdown(); }

std::future<std::vector<MQMessageQueue>> DefaultMQPullConsumer::queuesFor(const std::string& topic) {
  return impl_->queuesFor(topic);
}

std::future<int64_t> DefaultMQPullConsumer::queryOffset(const OffsetQuery& query) { return impl_->queryOffset(query); }

bool DefaultMQPullConsumer::pull(const PullMessageQuery& query, PullResult& pull_result) {
  auto callback = absl::make_unique<AwaitPullCallback>(pull_result);
  pull(query, callback.get());
  return callback->await();
}

void DefaultMQPullConsumer::pull(const PullMessageQuery& query, PullCallback* callback) {
  impl_->pull(query, callback);
}

void DefaultMQPullConsumer::setResourceNamespace(const std::string& resource_namespace) {
  impl_->resourceNamespace(resource_namespace);
}

void DefaultMQPullConsumer::setCredentialsProvider(std::shared_ptr<CredentialsProvider> credentials_provider) {
  impl_->setCredentialsProvider(std::move(credentials_provider));
}

void DefaultMQPullConsumer::setNamesrvAddr(const std::string& name_srv) {
  auto name_server_resolver = std::make_shared<StaticNameServerResolver>(name_srv);
  impl_->withNameServerResolver(name_server_resolver);
}

void DefaultMQPullConsumer::setNameServerListDiscoveryEndpoint(const std::string& discovery_endpoint) {
  auto name_server_resolver = std::make_shared<DynamicNameServerResolver>(discovery_endpoint, std::chrono::seconds(10));
  impl_->withNameServerResolver(name_server_resolver);
}

ROCKETMQ_NAMESPACE_END