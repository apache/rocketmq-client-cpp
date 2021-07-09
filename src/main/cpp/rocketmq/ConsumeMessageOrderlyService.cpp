#include "ConsumeMessageService.h"
#include "DefaultMQPushConsumerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

ConsumeMessageOrderlyService ::ConsumeMessageOrderlyService(
    const std::weak_ptr<DefaultMQPushConsumerImpl>&& consumer_impl_ptr, int thread_count,
    MQMessageListener* message_listener_ptr)
    : consumer_weak_ptr_(consumer_impl_ptr), message_listener_ptr_(message_listener_ptr), thread_count_(thread_count),
      pool_(absl::make_unique<grpc::DynamicThreadPool>(thread_count_)) {
  // Suppress field not used warning for now
  (void)message_listener_ptr_;
}

void ConsumeMessageOrderlyService::start() {
  // pool_ = std::make_shared<ThreadPool>(thread_count_);
}

void ConsumeMessageOrderlyService::shutdown() { ConsumeMessageService::shutdown(); }

void ConsumeMessageOrderlyService::submitConsumeTask(const ProcessQueueWeakPtr& process_queue, int32_t permits) {}

MessageListenerType ConsumeMessageOrderlyService::getConsumeMsgServiceListenerType() {
  return MessageListenerType::messageListenerOrderly;
}

void ConsumeMessageOrderlyService::stopThreadPool() {
  // do nothing, ThreadPool destructor will be invoked automatically
}

void ConsumeMessageOrderlyService::dispatch() {
  // Not implemented.
}

void ConsumeMessageOrderlyService::consumeTask(const ProcessQueueWeakPtr& process_queue,
                                               const std::vector<MQMessageExt>& msgs) {}

ROCKETMQ_NAMESPACE_END