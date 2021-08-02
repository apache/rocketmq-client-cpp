#include "ConsumeMessageService.h"
#include "DefaultMQPushConsumerImpl.h"
#include "MessageAccessor.h"
#include "ProcessQueue.h"
#include <chrono>
#include <functional>
#include <limits>

ROCKETMQ_NAMESPACE_BEGIN

ConsumeFifoMessageService ::ConsumeFifoMessageService(std::weak_ptr<PushConsumer> consumer, int thread_count,
                                                      MessageListener* message_listener)
    : ConsumeMessageService(std::move(consumer), thread_count, message_listener) {}

void ConsumeFifoMessageService::start() {
  ConsumeMessageService::start();
  State expected = State::STARTING;
  if (state_.compare_exchange_strong(expected, State::STARTED)) {
    SPDLOG_DEBUG("ConsumeMessageOrderlyService started");
  }
}

void ConsumeFifoMessageService::shutdown() {
  // Wait till consume-message-orderly-service has fully started; otherwise, we may potentially miss closing resources
  // in concurrent scenario.
  while (State::STARTING == state_.load(std::memory_order_relaxed)) {
    absl::SleepFor(absl::Milliseconds(10));
  }

  State expected = State::STARTED;
  if (state_.compare_exchange_strong(expected, STOPPING)) {
    ConsumeMessageService::shutdown();
    SPDLOG_INFO("ConsumeMessageOrderlyService shut down");
  }
}

void ConsumeFifoMessageService::submitConsumeTask0(const std::shared_ptr<PushConsumer>& consumer,
                                                   const ProcessQueueWeakPtr& process_queue,
                                                   const MQMessageExt& message) {
  // In case custom executor is used.
  const Executor& custom_executor = consumer->customExecutor();
  if (custom_executor) {
    std::function<void(void)> consume_task =
        std::bind(&ConsumeFifoMessageService::consumeTask, this, process_queue, message);
    custom_executor(consume_task);
    SPDLOG_DEBUG("Submit FIFO consume task to custom executor");
    return;
  }

  // submit batch message
  std::function<void(void)> consume_task =
      std::bind(&ConsumeFifoMessageService::consumeTask, this, process_queue, message);
  SPDLOG_DEBUG("Submit FIFO consume task to thread pool");
  pool_->Add(consume_task);
}

void ConsumeFifoMessageService::submitConsumeTask(const ProcessQueueWeakPtr& process_queue) {
  ProcessQueueSharedPtr process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    SPDLOG_INFO("Process queue has destructed");
    return;
  }

  auto consumer = consumer_.lock();
  if (!consumer) {
    SPDLOG_INFO("Consumer has destructed");
    return;
  }

  assert(1 == consumer->consumeBatchSize());

  if (process_queue_ptr->bindFifoConsumeTask()) {
    std::vector<MQMessageExt> messages;
    if (process_queue_ptr->take(consumer->consumeBatchSize(), messages)) {
      assert(1 == messages.size());
      submitConsumeTask0(consumer, process_queue, *messages.begin());
    }
  }
}

MessageListenerType ConsumeFifoMessageService::messageListenerType() { return MessageListenerType::FIFO; }

void ConsumeFifoMessageService::consumeTask(const ProcessQueueWeakPtr& process_queue, MQMessageExt& message) {
  ProcessQueueSharedPtr process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    return;
  }
  const std::string& topic = message.getTopic();
  ConsumeMessageResult result;
  std::shared_ptr<PushConsumer> consumer = consumer_.lock();
  // consumer might have been destructed.
  if (!consumer) {
    return;
  }

  std::shared_ptr<RateLimiter<10>> rate_limiter = rateLimiter(topic);
  if (rate_limiter) {
    rate_limiter->acquire();
    SPDLOG_DEBUG("Rate-limit permit acquired");
  }

  auto steady_start = std::chrono::steady_clock::now();

  try {
    assert(message_listener_);
    auto message_listener = dynamic_cast<FifoMessageListener*>(message_listener_);
    assert(message_listener);
    result = message_listener->consumeMessage(message);
  } catch (...) {
    result = ConsumeMessageResult::FAILURE;
    SPDLOG_ERROR("Business FIFO callback raised an exception when consumeMessage");
  }

  auto duration = std::chrono::steady_clock::now() - steady_start;

  // Log client consume-time costs
  SPDLOG_DEBUG("Business callback spent {}ms processing message[Topic={}, MessageId={}].",
               MixAll::millisecondsOf(duration), message.getTopic(), message.getMsgId());

  if (MessageModel::CLUSTERING == consumer->messageModel()) {
    if (result == ConsumeMessageResult::SUCCESS) {
      // Release message number and memory quota
      process_queue_ptr->release(message.getBody().size(), message.getQueueOffset());

      // Ensure current message is acked before moving to the next message.
      auto callback = std::bind(&ConsumeFifoMessageService::onAck, this, process_queue, message, std::placeholders::_1);
      consumer->ack(message, callback);
    } else {
      MessageAccessor::setDeliveryAttempt(message, message.getDeliveryAttempt() + 1);
      if (message.getDeliveryAttempt() < consumer->maxDeliveryAttempts()) {
        auto task = std::bind(&ConsumeFifoMessageService::scheduleConsumeTask, this, process_queue, message);
        consumer->schedule("Scheduled-Consume-FIFO-Message-Task", task, std::chrono::seconds(1));
      } else {
        auto callback = std::bind(&ConsumeFifoMessageService::onForwardToDeadLetterQueue, this, process_queue, message,
                                  std::placeholders::_1);
        consumer->forwardToDeadLetterQueue(message, callback);
      }
    }
  } else if (MessageModel::BROADCASTING == consumer->messageModel()) {
    process_queue_ptr->release(message.getBody().size(), message.getQueueOffset());
    int64_t committed_offset;
    if (process_queue_ptr->committedOffset(committed_offset)) {
      consumer->updateOffset(process_queue_ptr->getMQMessageQueue(), committed_offset);
    }
  }
}

void ConsumeFifoMessageService::onAck(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message, bool ok) {
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    SPDLOG_WARN("ProcessQueue has destructed.");
    return;
  }
  if (ok) {
    SPDLOG_DEBUG("Acknowledge FIFO message[MessageQueue={}, MsgId={}] OK", process_queue_ptr->simpleName(),
                 message.getMsgId());
    process_queue_ptr->unbindFifoConsumeTask();
    signalDispatcher();
  } else {
    SPDLOG_WARN("Failed to acknowledge FIFO message[MessageQueue={}, MsgId={}]", process_queue_ptr->simpleName(),
                message.getMsgId());
    auto consumer = consumer_.lock();
    if (!consumer) {
      SPDLOG_WARN("Consumer instance has destructed");
      return;
    }

    auto task = std::bind(&ConsumeFifoMessageService::scheduleAckTask, this, process_queue, message);
    int32_t duration = 100;
    consumer->schedule("Ack-FIFO-Message-On-Failure", task, std::chrono::milliseconds(duration));
    SPDLOG_INFO("Scheduled to ack message[Topic={}, MessageId={}] in {}ms", message.getTopic(), message.getMsgId(),
                duration);
  }
}

void ConsumeFifoMessageService::onForwardToDeadLetterQueue(const ProcessQueueWeakPtr& process_queue,
                                                           const MQMessageExt& message, bool ok) {
  if (ok) {
    SPDLOG_DEBUG("Forward message[Topic={}, MessagId={}] to DLQ OK", message.getTopic(), message.getMsgId());
    auto process_queue_ptr = process_queue.lock();
    if (process_queue_ptr) {
      process_queue_ptr->unbindFifoConsumeTask();
    }
    return;
  }

  SPDLOG_INFO("Failed to forward message[topic={}, MessageId={}] to DLQ", message.getTopic(), message.getMsgId());
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    SPDLOG_INFO("Abort further attempts considering its process queue has destructed");
    return;
  }

  auto consumer = consumer_.lock();
  assert(consumer);

  auto task = std::bind(&ConsumeFifoMessageService::scheduleForwardDeadLetterQueueTask, this, process_queue, message);
  consumer->schedule("Scheduled-Forward-DLQ-Task", task, std::chrono::milliseconds(100));
}

void ConsumeFifoMessageService::scheduleForwardDeadLetterQueueTask(const ProcessQueueWeakPtr& process_queue,
                                                                   const MQMessageExt& message) {
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    return;
  }
  auto consumer = consumer_.lock();
  assert(consumer);
  auto callback = std::bind(&ConsumeFifoMessageService::onForwardToDeadLetterQueue, this, process_queue, message,
                            std::placeholders::_1);
  consumer->forwardToDeadLetterQueue(message, callback);
}

void ConsumeFifoMessageService::scheduleAckTask(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message) {
  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    return;
  }

  auto callback = std::bind(&ConsumeFifoMessageService::onAck, this, process_queue, message, std::placeholders::_1);
  auto consumer = consumer_.lock();
  if (consumer) {
    consumer->ack(message, callback);
  }
}

void ConsumeFifoMessageService::scheduleConsumeTask(const ProcessQueueWeakPtr& process_queue,
                                                    const MQMessageExt& message) {
  auto consumer_ptr = consumer_.lock();
  if (!consumer_ptr) {
    return;
  }

  auto process_queue_ptr = process_queue.lock();
  if (!process_queue_ptr) {
    return;
  }

  submitConsumeTask0(consumer_ptr, process_queue_ptr, message);
  SPDLOG_INFO("Business callback failed to process FIFO messages. Re-submit consume task back to thread pool");
}

ROCKETMQ_NAMESPACE_END