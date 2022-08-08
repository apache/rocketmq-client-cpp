#pragma once

#include <memory>
#include <string>

#include "ONSConsumerAbstract.h"
#include "OrderListenerWrapper.h"
#include "ons/ONSFactory.h"
#include "ons/OrderConsumer.h"

#include "rocketmq/DefaultMQPushConsumer.h"

ONS_NAMESPACE_BEGIN

class OrderConsumerImpl : public OrderConsumer, public ONSConsumerAbstract {
public:
  explicit OrderConsumerImpl(const ONSFactoryProperty& factory_property);

  ~OrderConsumerImpl() override = default;

  void start() override;

  void shutdown() override;

  void subscribe(const std::string& topic, const std::string& subscribe_expression) override;

  void registerMessageListener(MessageOrderListener* listener) override;
};

ONS_NAMESPACE_END