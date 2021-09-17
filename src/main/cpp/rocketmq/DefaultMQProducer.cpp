#include "rocketmq/DefaultMQProducer.h"

#include <chrono>
#include <memory>

#include "absl/strings/str_split.h"

#include "DynamicNameServerResolver.h"
#include "MixAll.h"
#include "ProducerImpl.h"
#include "StaticNameServerResolver.h"

ROCKETMQ_NAMESPACE_BEGIN

DefaultMQProducer::DefaultMQProducer(const std::string& group_name)
    : impl_(std::make_shared<ProducerImpl>(group_name)) {}

void DefaultMQProducer::start() { impl_->start(); }

void DefaultMQProducer::shutdown() { impl_->shutdown(); }

std::chrono::milliseconds DefaultMQProducer::getSendMsgTimeout() const {
  return absl::ToChronoMilliseconds(impl_->getIoTimeout());
}

SendResult DefaultMQProducer::send(const MQMessage& message, const std::string& message_group) {
  return impl_->send(message, message_group);
}

void DefaultMQProducer::setSendMsgTimeout(std::chrono::milliseconds timeout) {
  impl_->setIoTimeout(absl::FromChrono(timeout));
}

void DefaultMQProducer::setNamesrvAddr(const std::string& name_server_address_list) {
  auto name_server_resolver = std::make_shared<StaticNameServerResolver>(name_server_address_list);
  impl_->withNameServerResolver(name_server_resolver);
}

void DefaultMQProducer::setNameServerListDiscoveryEndpoint(const std::string& discovery_endpoint) {
  auto name_server_resolver = std::make_shared<DynamicNameServerResolver>(discovery_endpoint, std::chrono::seconds(10));
  impl_->withNameServerResolver(name_server_resolver);
}

void DefaultMQProducer::setGroupName(const std::string& group_name) { impl_->setGroupName(group_name); }

void DefaultMQProducer::setInstanceName(const std::string& instance_name) { impl_->setInstanceName(instance_name); }

void DefaultMQProducer::enableTracing(bool enabled) { impl_->enableTracing(enabled); }

bool DefaultMQProducer::isTracingEnabled() { return impl_->isTracingEnabled(); }

SendResult DefaultMQProducer::send(const rocketmq::MQMessage& message, bool filter_active_broker) {
  return impl_->send(message);
}

SendResult DefaultMQProducer::send(const MQMessage& msg, const MQMessageQueue& mq) { return impl_->send(msg, mq); }

SendResult DefaultMQProducer::send(const MQMessage& msg, MessageQueueSelector* selector, void* arg) {
  return impl_->send(msg, selector, arg);
}

SendResult DefaultMQProducer::send(const MQMessage& message, MessageQueueSelector* selector, void* arg, int retry_times,
                                   bool select_active_broker) {
  return impl_->send(message, selector, arg, retry_times);
}

void DefaultMQProducer::send(const MQMessage& message, SendCallback* send_callback, bool select_active_broker) {
  impl_->send(message, send_callback);
}

void DefaultMQProducer::send(const MQMessage& message, const MQMessageQueue& message_queue,
                             SendCallback* send_callback) {
  impl_->send(message, message_queue, send_callback);
}

void DefaultMQProducer::send(const MQMessage& message, MessageQueueSelector* selector, void* arg,
                             SendCallback* send_callback) {
  impl_->send(message, selector, arg, send_callback);
}

void DefaultMQProducer::sendOneway(const MQMessage& message, bool select_active_broker) { impl_->sendOneway(message); }

void DefaultMQProducer::sendOneway(const MQMessage& message, const MQMessageQueue& message_queue) {
  impl_->sendOneway(message, message_queue);
}

void DefaultMQProducer::sendOneway(const MQMessage& message, MessageQueueSelector* selector, void* arg) {
  impl_->sendOneway(message, selector, arg);
}

void DefaultMQProducer::setLocalTransactionStateChecker(LocalTransactionStateCheckerPtr checker) {
  impl_->setLocalTransactionStateChecker(std::move(checker));
}

void DefaultMQProducer::setMaxAttemptTimes(int max_attempt_times) { impl_->maxAttemptTimes(max_attempt_times); }

int DefaultMQProducer::getMaxAttemptTimes() const { return impl_->maxAttemptTimes(); }

std::vector<MQMessageQueue> DefaultMQProducer::getTopicMessageQueueInfo(const std::string& topic) {
  return impl_->getTopicMessageQueueInfo(topic);
}

void DefaultMQProducer::setUnitName(std::string unit_name) { impl_->setUnitName(std::move(unit_name)); }

const std::string& DefaultMQProducer::getUnitName() { return impl_->getUnitName(); }

uint32_t DefaultMQProducer::compressBodyThreshold() const { return impl_->compressBodyThreshold(); }

void DefaultMQProducer::compressBodyThreshold(uint32_t threshold) { impl_->compressBodyThreshold(threshold); }

void DefaultMQProducer::setResourceNamespace(const std::string& resource_namespace) {
  impl_->resourceNamespace(resource_namespace);
}

void DefaultMQProducer::setCredentialsProvider(CredentialsProviderPtr credentials_provider) {
  impl_->setCredentialsProvider(std::move(credentials_provider));
}

void DefaultMQProducer::setRegion(const std::string& region) { impl_->region(region); }

ROCKETMQ_NAMESPACE_END