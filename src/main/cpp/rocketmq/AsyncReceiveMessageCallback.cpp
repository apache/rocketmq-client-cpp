#include "AsyncReceiveMessageCallback.h"
#include "ClientManagerImpl.h"
#include "ConsumeMessageType.h"
#include "LoggerImpl.h"
#include "PushConsumer.h"

ROCKETMQ_NAMESPACE_BEGIN

AsyncReceiveMessageCallback::AsyncReceiveMessageCallback(ProcessQueueWeakPtr process_queue)
    : process_queue_(std::move(process_queue)) {
  receive_message_later_ = std::bind(&AsyncReceiveMessageCallback::checkThrottleThenReceive, this);
}

void AsyncReceiveMessageCallback::onSuccess(ReceiveMessageResult& result) {
  ProcessQueueSharedPtr process_queue_shared_ptr = process_queue_.lock();
  if (!process_queue_shared_ptr) {
    SPDLOG_WARN("Process queue has been released. Drop PopResult: {}", result.toString());
    return;
  }

  std::shared_ptr<PushConsumer> impl = process_queue_shared_ptr->getConsumer().lock();
  if (!impl->active()) {
    return;
  }

  auto receive_message_action = process_queue_shared_ptr->getConsumer().lock()->receiveMessageAction();

  switch (result.status()) {
  case ReceiveMessageStatus::OK:
    SPDLOG_DEBUG("Receive messages from broker[host={}] returns with status=FOUND, msgListSize={}, queue={}",
                 result.sourceHost(), result.getMsgFoundList().size(), process_queue_shared_ptr->simpleName());
    process_queue_shared_ptr->cacheMessages(result.getMsgFoundList());
    impl->getConsumeMessageService()->signalDispatcher();

    if (ReceiveMessageAction::PULL == receive_message_action) {
      process_queue_shared_ptr->nextOffset(result.next_offset_);
    }
    checkThrottleThenReceive();
    break;
  case ReceiveMessageStatus::DATA_CORRUPTED:
    if (ReceiveMessageAction::POLLING == receive_message_action) {
      process_queue_shared_ptr->cacheMessages(result.messages_);
      impl->getConsumeMessageService()->signalDispatcher();
    }
    checkThrottleThenReceive();
    break;

  case ReceiveMessageStatus::OUT_OF_RANGE:
    assert(ReceiveMessageAction::PULL == receive_message_action);
    process_queue_shared_ptr->nextOffset(result.next_offset_);
    checkThrottleThenReceive();
    break;
  case ReceiveMessageStatus::DEADLINE_EXCEEDED:
    SPDLOG_DEBUG("Receive messages from broker[host={}] returns with status=DEADLINE_EXCEEDED, queue={}",
                 result.sourceHost(), process_queue_shared_ptr->simpleName());
    checkThrottleThenReceive();
    break;
  case ReceiveMessageStatus::INTERNAL:
    SPDLOG_DEBUG("Receive messages from broker[host={}] returns with status=UNKNOWN, queue={}", result.sourceHost(),
                 process_queue_shared_ptr->simpleName());
    receiveMessageLater();
    break;
  case ReceiveMessageStatus::RESOURCE_EXHAUSTED:
    SPDLOG_DEBUG("Receive messages from broker[host={}] returns with status=RESOURCE_EXHAUSTED, queue={}",
                 result.sourceHost(), process_queue_shared_ptr->simpleName());
    receiveMessageLater();
    break;
  case ReceiveMessageStatus::NOT_FOUND:
    SPDLOG_DEBUG("Receive messages from broker[host={}] returns with status=NOT_FOUND, queue={}", result.sourceHost(),
                 process_queue_shared_ptr->simpleName());
    receiveMessageLater();
    break;
  default:
    SPDLOG_WARN("Unknown receive message status: {} from broker[host={}], queue={}", result.status(),
                result.sourceHost(), process_queue_shared_ptr->simpleName());
    receiveMessageLater();
    break;
  }
}

const char* AsyncReceiveMessageCallback::RECEIVE_LATER_TASK_NAME = "receive-later-task";

void AsyncReceiveMessageCallback::checkThrottleThenReceive() {
  auto process_queue = process_queue_.lock();
  if (!process_queue) {
    SPDLOG_WARN("Process queue should have been destructed");
    return;
  }

  if (process_queue->shouldThrottle()) {
    SPDLOG_INFO("Number of messages in {} exceeds throttle threshold. Receive messages later.",
                process_queue->simpleName());
    process_queue->syncIdleState();
    receiveMessageLater();
  } else {
    // Receive message immediately
    receiveMessageImmediately();
  }
}

void AsyncReceiveMessageCallback::onException(MQException& e) {
  auto process_queue_ptr = process_queue_.lock();
  if (process_queue_ptr) {
    SPDLOG_WARN("pop message error:{}, pop message later. Queue={}", e.what(), process_queue_ptr->simpleName());
    // pop message later
    receiveMessageLater();
  }
}

void AsyncReceiveMessageCallback::receiveMessageLater() {
  auto process_queue = process_queue_.lock();
  if (!process_queue) {
    return;
  }

  auto client_instance = process_queue->getClientManager();
  std::weak_ptr<AsyncReceiveMessageCallback> receive_callback_weak_ptr(shared_from_this());

  auto task = [receive_callback_weak_ptr]() {
    auto async_receive_ptr = receive_callback_weak_ptr.lock();
    if (async_receive_ptr) {
      async_receive_ptr->checkThrottleThenReceive();
    }
  };

  client_instance->getScheduler().schedule(task, RECEIVE_LATER_TASK_NAME, std::chrono::seconds(1),
                                           std::chrono::seconds(0));
}

void AsyncReceiveMessageCallback::receiveMessageImmediately() {
  ProcessQueueSharedPtr process_queue_shared_ptr = process_queue_.lock();
  if (!process_queue_shared_ptr) {
    SPDLOG_INFO("ProcessQueue has been released. Ignore further receive message request-response cycles");
    return;
  }

  std::shared_ptr<PushConsumer> impl = process_queue_shared_ptr->getConsumer().lock();
  if (!impl) {
    SPDLOG_INFO("Owner of ProcessQueue[{}] has been released. Ignore further receive message request-response cycles",
                process_queue_shared_ptr->simpleName());
    return;
  }
  impl->receiveMessage(process_queue_shared_ptr->getMQMessageQueue(), process_queue_shared_ptr->getFilterExpression());
}

ROCKETMQ_NAMESPACE_END