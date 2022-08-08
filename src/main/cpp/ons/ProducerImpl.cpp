#include "ProducerImpl.h"

#include <chrono>
#include <cstdlib>
#include <memory>
#include <system_error>

#include "ONSUtil.h"
#include "absl/strings/ascii.h"
#include "ons/ONSClientException.h"
#include "rocketmq/CredentialsProvider.h"
#include "rocketmq/RocketMQ.h"

ONS_NAMESPACE_BEGIN

ProducerImpl::ProducerImpl(const ONSFactoryProperty& factory_property)
    : ONSClientAbstract(factory_property), producer_(std::string(factory_property.getProducerId())) {

  auto send_msg_timeout = factory_property.getSendMsgTimeout();
  if (send_msg_timeout.count()) {
    producer_.setSendMsgTimeout(send_msg_timeout);
  }

  absl::string_view instanceName = absl::StripAsciiWhitespace(factory_property.getInstanceId());
  if (instanceName.empty()) {
    producer_.setInstanceName(buildInstanceName());
  } else {
    producer_.setInstanceName(std::string(instanceName));
  }
  int send_msg_retry_times = factory_property.getSendMsgRetryTimes();
  if (send_msg_retry_times > 0) {
    producer_.setMaxAttemptTimes(send_msg_retry_times);
  }

  auto credentials_provider = std::make_shared<ROCKETMQ_NAMESPACE::StaticCredentialsProvider>(
      factory_property.getAccessKey(), factory_property.getSecretKey());
  producer_.setCredentialsProvider(credentials_provider);

  if (access_point_) {
    std::string&& resource_namespace = access_point_.resourceNamespace();
    producer_.setResourceNamespace(resource_namespace);
    producer_.setNamesrvAddr(access_point_.nameServerAddress());
  } else if (!factory_property.getNameSrvAddr().empty()) {
    producer_.setNamesrvAddr(factory_property.getNameSrvAddr());
  } else if (!factory_property.getNameSrvDomain().empty()) {
    producer_.setNameServerListDiscoveryEndpoint(factory_property.getNameSrvDomain());
  }

  bool traceSwitchOn = factory_property.getOnsTraceSwitch();
  producer_.enableTracing(traceSwitchOn);
}

void ProducerImpl::start() {
  producer_.start();
}

void ProducerImpl::shutdown() {
  producer_.shutdown();
}

SendResultONS ProducerImpl::send(Message& message) {
  ROCKETMQ_NAMESPACE::MQMessage mq_message = ONSUtil::get().msgConvert(message);

  ROCKETMQ_NAMESPACE::SendResult send_result = producer_.send(mq_message);

  SendResultONS send_result_ons;
  send_result_ons.setMessageId(send_result.getMsgId());
  return send_result_ons;
}

SendResultONS ProducerImpl::send(Message& message, std::error_code& ec) noexcept {
  ROCKETMQ_NAMESPACE::MQMessage mq_message = ONSUtil::get().msgConvert(message);
  ROCKETMQ_NAMESPACE::SendResult send_result = producer_.send(mq_message, ec);
  if (ec) {
    return {};
  }
  SendResultONS send_result_ons;
  send_result_ons.setMessageId(send_result.getMsgId());
  return send_result_ons;
}

void ProducerImpl::sendAsync(Message& message, SendCallbackONS* callback) noexcept {
  ROCKETMQ_NAMESPACE::MQMessage mq_message = ONSUtil::get().msgConvert(message);

  if (!callback) {
    abort();
  }

  std::shared_ptr<SendCallbackONSWrapper> send_callback_ons_wrapper_shared_ptr = nullptr;
  {
    absl::MutexLock lock(&callbacks_mtx_);
    if (nullptr == callbacks_[callback]) {
      send_callback_ons_wrapper_shared_ptr = std::make_shared<SendCallbackONSWrapper>(callback);
      callbacks_[callback] = send_callback_ons_wrapper_shared_ptr;
    }
    send_callback_ons_wrapper_shared_ptr = callbacks_[callback];
  }
  producer_.send(mq_message, send_callback_ons_wrapper_shared_ptr.get(), true);
}

void ProducerImpl::sendOneway(Message& message) noexcept {
  ROCKETMQ_NAMESPACE::MQMessage mq_message = ONSUtil::get().msgConvert(message);
  producer_.sendOneway(mq_message);
}

ROCKETMQ_NAMESPACE::MQMessageQueue ProducerImpl::messageQueueConvert(const MessageQueueONS& message_queue_ons) {
  ROCKETMQ_NAMESPACE::MQMessageQueue mq_message_queue;
  mq_message_queue.setBrokerName(message_queue_ons.getBrokerName());
  mq_message_queue.setQueueId(message_queue_ons.getQueueId());
  mq_message_queue.setTopic(message_queue_ons.getTopic());
  return mq_message_queue;
}

ONS_NAMESPACE_END