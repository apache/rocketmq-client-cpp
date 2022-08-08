#pragma once

#include <system_error>

#include "ONSClientAbstract.h"
#include "SendCallbackONSWrapper.h"
#include "absl/container/flat_hash_map.h"
#include "ons/ONSFactory.h"
#include "ons/Producer.h"
#include "rocketmq/DefaultMQProducer.h"
#include "rocketmq/RocketMQ.h"

ONS_NAMESPACE_BEGIN

class ProducerImpl : public Producer, public ONSClientAbstract {
public:
  explicit ProducerImpl(const ONSFactoryProperty& factory_property);

  ~ProducerImpl() override = default;

  void start() override;

  void shutdown() override;

  SendResultONS send(Message& message) noexcept(false) override;

  SendResultONS send(Message& message, std::error_code& ec) noexcept override;

  void sendAsync(Message& message, SendCallbackONS* callback) noexcept override;

  void sendOneway(Message& message) noexcept override;

private:
  static ROCKETMQ_NAMESPACE::MQMessageQueue messageQueueConvert(const MessageQueueONS& message_queue_ons);

  ROCKETMQ_NAMESPACE::DefaultMQProducer producer_;

  absl::flat_hash_map<SendCallbackONS*, std::shared_ptr<SendCallbackONSWrapper>> callbacks_ GUARDED_BY(callbacks_mtx_);

  absl::Mutex callbacks_mtx_; // protects clients_
};

ONS_NAMESPACE_END