#pragma once

#include <functional>
#include <memory>

#include "ConsumeMessageService.h"
#include "Consumer.h"
#include "ProcessQueue.h"
#include "rocketmq/Executor.h"
#include "rocketmq/MessageListener.h"
#include "rocketmq/MessageModel.h"

ROCKETMQ_NAMESPACE_BEGIN

class PushConsumer : virtual public Consumer {
public:
  ~PushConsumer() override = default;

  virtual void iterateProcessQueue(const std::function<void(ProcessQueueSharedPtr)>& cb) = 0;

  virtual MessageModel messageModel() const = 0;

  virtual void ack(const MQMessageExt& msg, const std::function<void(bool)>& callback) = 0;

  virtual void forwardToDeadLetterQueue(const MQMessageExt& message, const std::function<void(bool)>& cb) = 0;

  virtual const Executor& customExecutor() const = 0;

  virtual uint32_t consumeBatchSize() const = 0;

  virtual int32_t maxDeliveryAttempts() const = 0;

  virtual void updateOffset(const MQMessageQueue& message_queue, int64_t offset) = 0;

  virtual void nack(const MQMessageExt& message, const std::function<void(bool)>& callback) = 0;

  virtual std::shared_ptr<ConsumeMessageService> getConsumeMessageService() = 0;

  virtual bool receiveMessage(const MQMessageQueue& message_queue, const FilterExpression& filter_expression) = 0;

  virtual MessageListener* messageListener() = 0;
};

using PushConsumerSharedPtr = std::shared_ptr<PushConsumer>;
using PushConsumerWeakPtr = std::weak_ptr<PushConsumer>;

ROCKETMQ_NAMESPACE_END