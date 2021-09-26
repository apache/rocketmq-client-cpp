#pragma once

#include "ProcessQueue.h"
#include "ReceiveMessageCallback.h"
#include <system_error>

ROCKETMQ_NAMESPACE_BEGIN

class AsyncReceiveMessageCallback : public ReceiveMessageCallback,
                                    public std::enable_shared_from_this<AsyncReceiveMessageCallback> {
public:
  explicit AsyncReceiveMessageCallback(ProcessQueueWeakPtr process_queue);

  ~AsyncReceiveMessageCallback() override = default;

  void onSuccess(ReceiveMessageResult& result) override;

  void onFailure(const std::error_code& ec) override;

  void receiveMessageLater();

  void receiveMessageImmediately();

private:
  /**
   * Hold a weak_ptr to ProcessQueue. Once ProcessQueue was released, stop the
   * pop-cycle immediately.
   */
  ProcessQueueWeakPtr process_queue_;

  std::function<void(void)> receive_message_later_;

  void checkThrottleThenReceive();

  static const char* RECEIVE_LATER_TASK_NAME;
};

ROCKETMQ_NAMESPACE_END