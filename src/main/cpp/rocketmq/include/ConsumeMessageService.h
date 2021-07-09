#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "ProcessQueue.h"
#include "RateLimiter.h"
#include "rocketmq/MQMessageListener.h"
#include "rocketmq/State.h"
#include "src/cpp/server/dynamic_thread_pool.h"

ROCKETMQ_NAMESPACE_BEGIN

class DefaultMQPushConsumerImpl;

class ConsumeMessageService {
public:
  ConsumeMessageService() = default;

  virtual ~ConsumeMessageService() = default;

  /**
   * Make it noncopyable.
   */
  ConsumeMessageService(const ConsumeMessageService& other) = delete;
  ConsumeMessageService& operator=(const ConsumeMessageService& other) = delete;

  virtual void start() = 0;

  virtual void shutdown();

  /**
   *
   * @param process_queue_ptr
   * @param permits submit at most permits messages to consume threads; if permits is negative, all messages in process
   * queue should be submitted.
   */
  virtual void submitConsumeTask(const ProcessQueueWeakPtr& process_queue_ptr, int32_t permits) = 0;

  virtual MessageListenerType getConsumeMsgServiceListenerType() = 0;
  virtual void stopThreadPool() = 0;

  virtual void dispatch() = 0;

  void throttle(const std::string& topic, uint32_t threshold);

  bool hasConsumeRateLimiter(const std::string& topic) const;

  std::shared_ptr<RateLimiter<10>> rateLimiter(const std::string& topic) const;

protected:
  RateLimiterObserver rate_limiter_observer_;

  mutable absl::flat_hash_map<std::string, std::shared_ptr<RateLimiter<10>>>
      rate_limiter_table_ GUARDED_BY(rate_limiter_table_mtx_);
  mutable absl::Mutex rate_limiter_table_mtx_; // Protects rate_limiter_table_
};

class ConsumeMessageConcurrentlyService : public ConsumeMessageService {
public:
  ConsumeMessageConcurrentlyService(std::weak_ptr<DefaultMQPushConsumerImpl> consumer, int thread_count,
                                    MQMessageListener* message_listener_ptr);

  ~ConsumeMessageConcurrentlyService() override = default;

  void start() override;
  void shutdown() override;

  void submitConsumeTask(const ProcessQueueWeakPtr& process_queue, int32_t permits) override;

  MessageListenerType getConsumeMsgServiceListenerType() override;
  void stopThreadPool() override;

  void dispatch() override;

private:
  void consumeTask(const ProcessQueueWeakPtr& process_queue, const std::vector<MQMessageExt>& msgs);

  void dispatch0();

  int thread_count_;
  MQMessageListener* message_listener_ptr_;
  std::atomic<State> state_;
  std::unique_ptr<grpc::ThreadPoolInterface> pool_;
  std::weak_ptr<DefaultMQPushConsumerImpl> consumer_weak_ptr_;

  absl::Mutex dispatch_mtx_;
  std::thread dispatch_thread_;
  absl::CondVar dispatch_cv_;
};

class ConsumeMessageOrderlyService : public ConsumeMessageService {
public:
  ConsumeMessageOrderlyService(const std::weak_ptr<DefaultMQPushConsumerImpl>&& consumer_impl_ptr, int thread_count,
                               MQMessageListener* message_listener_ptr);
  void start() override;
  void shutdown() override;

  void submitConsumeTask(const ProcessQueueWeakPtr& process_queue, int32_t permits) override;

  MessageListenerType getConsumeMsgServiceListenerType() override;
  void stopThreadPool() override;
  void dispatch() override;

private:
  void consumeTask(const ProcessQueueWeakPtr& process_queue, const std::vector<MQMessageExt>& msgs);
  std::weak_ptr<DefaultMQPushConsumerImpl> consumer_weak_ptr_;
  MQMessageListener* message_listener_ptr_;
  int thread_count_;
  std::unique_ptr<grpc::ThreadPoolInterface> pool_;
};

ROCKETMQ_NAMESPACE_END