#include "OrderConsumerImpl.h"
#include "OrderListenerWrapper.h"

ONS_NAMESPACE_BEGIN

OrderConsumerImpl::OrderConsumerImpl(const ONSFactoryProperty& factory_property)
    : ONSConsumerAbstract(factory_property) {}

void OrderConsumerImpl::start() { ONSConsumerAbstract::start(); }

void OrderConsumerImpl::shutdown() { ONSConsumerAbstract::shutdown(); }

void OrderConsumerImpl::subscribe(const std::string& topic, const std::string& expression) {
  ONSConsumerAbstract::subscribe(topic, expression);
}

void OrderConsumerImpl::registerMessageListener(MessageOrderListener* listener) {
  std::unique_ptr<ROCKETMQ_NAMESPACE::MessageListener> wrapped_listener =
      absl::make_unique<OrderListenerWrapper>(listener);
  ONSConsumerAbstract::registerMessageListener(std::move(wrapped_listener));
}

ONS_NAMESPACE_END