#include "rocketmq/DefaultMQProducer.h"

#include <chrono>
#include <memory>
#include <system_error>
#include <utility>

#include "absl/strings/str_split.h"

#include "DynamicNameServerResolver.h"
#include "MixAll.h"
#include "ProducerImpl.h"
#include "StaticNameServerResolver.h"
#include "rocketmq/Transaction.h"

ROCKETMQ_NAMESPACE_BEGIN

DefaultMQProducer::DefaultMQProducer(const std::string& group_name)
    : impl_(std::make_shared<ProducerImpl>(group_name)) {}

void DefaultMQProducer::start() { impl_->start(); }

void DefaultMQProducer::shutdown() { impl_->shutdown(); }

std::chrono::milliseconds DefaultMQProducer::getSendMsgTimeout() const {
  return absl::ToChronoMilliseconds(impl_->getIoTimeout());
}

SendResult DefaultMQProducer::send(MQMessage& message, const std::string& message_group) {
  message.bindMessageGroup(message_group);
  std::error_code ec;
  auto&& send_result = impl_->send(message, ec);
  if (ec) {
    THROW_MQ_EXCEPTION(MQClientException, ec.message(), ec.value());
  }
  return std::move(send_result);
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

SendResult DefaultMQProducer::send(const MQMessage& message, bool filter_active_broker) {
  std::error_code ec;
  auto&& send_result = impl_->send(message, ec);
  if (ec) {
    THROW_MQ_EXCEPTION(MQClientException, ec.message(), ec.value());
  }
  return std::move(send_result);
}

SendResult DefaultMQProducer::send(const MQMessage& message, std::error_code& ec) noexcept {
  return impl_->send(message, ec);
}

SendResult DefaultMQProducer::send(MQMessage& msg, const MQMessageQueue& mq) {
  msg.bindMessageQueue(mq);
  std::error_code ec;
  auto&& send_result = impl_->send(msg, ec);
  if (ec) {
    THROW_MQ_EXCEPTION(MQClientException, ec.message(), ec.value());
  }
  return std::move(send_result);
}

SendResult DefaultMQProducer::send(MQMessage& msg, MessageQueueSelector* selector, void* arg) {
  std::error_code ec;
  auto&& list = impl_->listMessageQueue(msg.getTopic(), ec);
  if (ec) {
    THROW_MQ_EXCEPTION(MQClientException, ec.message(), ec.value());
  }

  auto&& message_queue = selector->select(list, msg, arg);
  msg.bindMessageQueue(message_queue);

  auto&& send_result = impl_->send(msg, ec);
  if (ec) {
    THROW_MQ_EXCEPTION(MQClientException, ec.message(), ec.value());
  }
  return std::move(send_result);
}

SendResult DefaultMQProducer::send(MQMessage& message, MessageQueueSelector* selector, void* arg, int retry_times,
                                   bool select_active_broker) {
  return send(message, selector, arg);
}

void DefaultMQProducer::send(const MQMessage& message, SendCallback* send_callback, bool select_active_broker) {
  impl_->send(message, send_callback);
}

void DefaultMQProducer::send(MQMessage& message, const MQMessageQueue& message_queue, SendCallback* send_callback) {
  message.bindMessageQueue(message_queue);
  impl_->send(message, send_callback);
}

void DefaultMQProducer::send(MQMessage& message, MessageQueueSelector* selector, void* arg,
                             SendCallback* send_callback) {
  std::error_code ec;

  // TODO: make querying route async
  auto&& list = impl_->listMessageQueue(message.getTopic(), ec);
  if (ec) {
    send_callback->onFailure(ec);
  }

  if (list.empty()) {
    send_callback->onFailure(ErrorCode::ServiceUnavailable);
  }

  auto&& message_queue = selector->select(list, message, arg);
  message.bindMessageQueue(message_queue);

  impl_->send(message, send_callback);
}

void DefaultMQProducer::sendOneway(const MQMessage& message, bool select_active_broker) {
  std::error_code ec;
  impl_->sendOneway(message, ec);
}

void DefaultMQProducer::sendOneway(MQMessage& message, const MQMessageQueue& message_queue) {
  message.bindMessageQueue(message_queue);
  std::error_code ec;
  impl_->sendOneway(message, ec);
  if (ec) {
    SPDLOG_INFO("Failed to send message in one-way: {}", ec.message());
  }
}

void DefaultMQProducer::sendOneway(MQMessage& message, MessageQueueSelector* selector, void* arg) {
  std::error_code ec;
  auto&& list = impl_->listMessageQueue(message.getTopic(), ec);
  if (ec) {
    THROW_MQ_EXCEPTION(MQClientException, ec.message(), ec.value());
  }

  if (list.empty()) {
    ec = ErrorCode::ServiceUnavailable;
    THROW_MQ_EXCEPTION(MQClientException, ec.message(), ec.value());
  }

  auto&& message_queue = selector->select(list, message, arg);
  message.bindMessageQueue(message_queue);
  impl_->sendOneway(message, ec);
}

void DefaultMQProducer::setLocalTransactionStateChecker(LocalTransactionStateCheckerPtr checker) {
  impl_->setLocalTransactionStateChecker(std::move(checker));
}

void DefaultMQProducer::setMaxAttemptTimes(int max_attempt_times) { impl_->maxAttemptTimes(max_attempt_times); }

int DefaultMQProducer::getMaxAttemptTimes() const { return impl_->maxAttemptTimes(); }

std::vector<MQMessageQueue> DefaultMQProducer::getTopicMessageQueueInfo(const std::string& topic) {
  std::error_code ec;
  auto&& list = impl_->listMessageQueue(topic, ec);
  if (ec) {
    THROW_MQ_EXCEPTION(MQClientException, ec.message(), ec.value());
  }
  return std::move(list);
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

TransactionPtr DefaultMQProducer::prepare(MQMessage& message) {
  std::error_code ec;
  auto transaction = impl_->prepare(message, ec);
  if (ec) {
    THROW_MQ_EXCEPTION(MQClientException, ec.message(), ec.value());
  }
  return transaction;
}

ROCKETMQ_NAMESPACE_END