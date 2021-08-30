#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "ProcessQueue.h"
#include "RateLimiter.h"
#include "ThreadPool.h"
#include "absl/container/flat_hash_map.h"
#include "rocketmq/MessageListener.h"
#include "rocketmq/State.h"

ROCKETMQ_NAMESPACE_BEGIN

class PushConsumer;

class ConsumeMessageService {
public:
  ConsumeMessageService(std::weak_ptr<PushConsumer> consumer, int thread_count, MessageListener* message_listener);

  virtual ~ConsumeMessageService() = default;

  /**
   * Make it noncopyable.
   */
  ConsumeMessageService(const ConsumeMessageService& other) = delete;
  ConsumeMessageService& operator=(const ConsumeMessageService& other) = delete;

  /**
   * Start the dispatcher thread, which will dispatch messages in process queue to thread pool in form of runnable
   * functor.
   */
  virtual void start();

  /**
   * Stop the dispatcher thread and then reset the thread pool.
   */
  virtual void shutdown();

  virtual void submitConsumeTask(const ProcessQueueWeakPtr& process_queue_ptr) = 0;

  virtual MessageListenerType messageListenerType() = 0;

  /**
   * Signal dispatcher thread to check new pending messages.
   */
  void signalDispatcher();

  /**
   * Set throttle threshold per topic.
   *
   * @param topic
   * @param threshold
   */
  void throttle(const std::string& topic, uint32_t threshold);

  bool hasConsumeRateLimiter(const std::string& topic) const LOCKS_EXCLUDED(rate_limiter_table_mtx_);

  std::shared_ptr<RateLimiter<10>> rateLimiter(const std::string& topic) const LOCKS_EXCLUDED(rate_limiter_table_mtx_);

protected:
  RateLimiterObserver rate_limiter_observer_;

  mutable absl::flat_hash_map<std::string, std::shared_ptr<RateLimiter<10>>>
      rate_limiter_table_ GUARDED_BY(rate_limiter_table_mtx_);
  mutable absl::Mutex rate_limiter_table_mtx_; // Protects rate_limiter_table_

  std::atomic<State> state_;

  int thread_count_;
  std::unique_ptr<ThreadPool> pool_;
  std::weak_ptr<PushConsumer> consumer_;

  absl::Mutex dispatch_mtx_;
  std::thread dispatch_thread_;
  absl::CondVar dispatch_cv_;

  MessageListener* message_listener_;

  /**
   * Dispatch messages to thread pool. Implementation of this function should be sub-class specific.
   */
  void dispatch();
};

class ConsumeStandardMessageService : public ConsumeMessageService {
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

class ConsumeFifoMessageService : public ConsumeMessageService,
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

  void onAck(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message, bool ok);

  void scheduleConsumeTask(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message);

  void onForwardToDeadLetterQueue(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message, bool ok);

  void scheduleForwardDeadLetterQueueTask(const ProcessQueueWeakPtr& process_queue, const MQMessageExt& message);
};

ROCKETMQ_NAMESPACE_END