#include "ConsumeMessageServiceBase.h"

ROCKETMQ_NAMESPACE_BEGIN

class ConsumeFifoMessageService : public ConsumeMessageServiceBase,
                                  public std::enable_shared_from_this<ConsumeFifoMessageService> {
public:
  ConsumeFifoMessageService(std::weak_ptr<PushConsumer> consumer, int thread_count, MessageListener* message_listener);
  void start() override;

  void shutdown() override;

  void submitConsumeTask(const ProcessQueueWeakPtr& process_queue) override;

  MessageListenerType messageListenerType() override;

private:
  void consumeTask(const ProcessQueueWeakPtr& process_queue, MQMessageExt& message);

  void submitConsumeTask0(const std::shared_ptr<PushConsumer>& consumer, const ProcessQueueWeakPtr& process_queue,
                          const MQMessageExt& message);

  void scheduleAckTask(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message);

  void onAck(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message, const std::error_code& ec);

  void scheduleConsumeTask(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message);

  void onForwardToDeadLetterQueue(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message, bool ok);

  void scheduleForwardDeadLetterQueueTask(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message);
};

ROCKETMQ_NAMESPACE_END