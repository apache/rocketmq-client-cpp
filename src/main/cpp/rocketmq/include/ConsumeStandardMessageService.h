#pragma once

#include "ConsumeMessageServiceBase.h"

ROCKETMQ_NAMESPACE_BEGIN

class ConsumeStandardMessageService : public ConsumeMessageServiceBase {
public:
  ConsumeStandardMessageService(std::weak_ptr<PushConsumer> consumer, int thread_count,
                                MessageListener* message_listener_ptr);

  ~ConsumeStandardMessageService() override = default;

  void start() override;

  void shutdown() override;

  void submitConsumeTask(const ProcessQueueWeakPtr& process_queue) override;

  MessageListenerType messageListenerType() override;

private:
  void consumeTask(const ProcessQueueWeakPtr& process_queue, const std::vector<MQMessageExt>& msgs);
};

ROCKETMQ_NAMESPACE_END