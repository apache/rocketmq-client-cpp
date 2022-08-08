#include "OrderProducerImpl.h"

#include "absl/strings/ascii.h"

#include "ONSUtil.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/RocketMQ.h"

ONS_NAMESPACE_BEGIN

OrderProducerImpl::OrderProducerImpl(const ONSFactoryProperty& factory_property)
    : ONSClientAbstract(factory_property), producer_(factory_property.getGroupId()) {
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
}

void OrderProducerImpl::start() {
  producer_.start();
}

void OrderProducerImpl::shutdown() {
  producer_.shutdown();
}

SendResultONS OrderProducerImpl::send(Message& msg, std::string message_group) {
  ROCKETMQ_NAMESPACE::MQMessage message = ons::ONSUtil::get().msgConvert(msg);
  ROCKETMQ_NAMESPACE::SendResult send_result = producer_.send(message, message_group);
  SendResultONS ons_send_result = SendResultONS();
  ons_send_result.setMessageId(send_result.getMsgId());
  return ons_send_result;
}

ONS_NAMESPACE_END