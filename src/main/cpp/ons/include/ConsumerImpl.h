#pragma once

#include <string>

#include "MessageListenerWrapper.h"
#include "ONSConsumerAbstract.h"
#include "absl/container/flat_hash_map.h"
#include "ons/ONSFactory.h"
#include "rocketmq/DefaultMQPushConsumer.h"

ONS_NAMESPACE_BEGIN

class ConsumerImpl : public PushConsumer, public ONSConsumerAbstract {
public:
  explicit ConsumerImpl(const ONSFactoryProperty& factory_property);

  ~ConsumerImpl() override = default;

  void start() override;

  void shutdown() override;

  void subscribe(const std::string& topic, const std::string& sub_expression) override;

  void registerMessageListener(MessageListener* listener) override;

  void withOffsetStore(std::unique_ptr<OffsetStore> offset_store) override;
};

ONS_NAMESPACE_END