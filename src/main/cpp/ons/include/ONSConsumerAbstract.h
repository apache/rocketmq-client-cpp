#pragma once

#include <memory>

#include "ONSClientAbstract.h"
#include "rocketmq/DefaultMQPushConsumer.h"
#include "rocketmq/MessageListener.h"
#include "rocketmq/RocketMQ.h"

ONS_NAMESPACE_BEGIN

class ONSConsumerAbstract : public ONSClientAbstract {
public:
  explicit ONSConsumerAbstract(const ONSFactoryProperty& factory_property);

  void start() override;

  void shutdown() override;

protected:
  void subscribe(absl::string_view topic, absl::string_view sub_expression);

  void registerMessageListener(std::unique_ptr<ROCKETMQ_NAMESPACE::MessageListener> message_listener);

  ROCKETMQ_NAMESPACE::DefaultMQPushConsumer consumer_;

  std::unique_ptr<ROCKETMQ_NAMESPACE::MessageListener> message_listener_;
};

ONS_NAMESPACE_END