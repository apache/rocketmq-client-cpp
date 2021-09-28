#pragma once

#include <cstdint>

#include "ProcessQueue.h"
#include "rocketmq/MessageListener.h"

ROCKETMQ_NAMESPACE_BEGIN

class ConsumeMessageService {
public:
  virtual ~ConsumeMessageService() = default;

  /**
   * Start the dispatcher thread, which will dispatch messages in process queue to thread pool in form of runnable
   * functor.
   */
  virtual void start() = 0;

  /**
   * Stop the dispatcher thread and then reset the thread pool.
   */
  virtual void shutdown() = 0;

  virtual void submitConsumeTask(const ProcessQueueWeakPtr& process_queue_ptr) = 0;

  virtual MessageListenerType messageListenerType() = 0;

  virtual void signalDispatcher() = 0;

  virtual void throttle(const std::string& topic, std::uint32_t threshold) = 0;
};

ROCKETMQ_NAMESPACE_END