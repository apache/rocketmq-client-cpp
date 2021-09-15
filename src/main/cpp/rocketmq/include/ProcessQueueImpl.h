#pragma once

#include <apache/rocketmq/v1/definition.pb.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <set>

#include "Assignment.h"
#include "ClientManager.h"
#include "FilterExpression.h"
#include "MixAll.h"
#include "ProcessQueue.h"
#include "ReceiveMessageCallback.h"
#include "TopicAssignmentInfo.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "apache/rocketmq/v1/service.pb.h"
#include "rocketmq/ConsumeType.h"
#include "rocketmq/MQMessageExt.h"
#include "rocketmq/MQMessageQueue.h"
#include "gtest/gtest_prod.h"

ROCKETMQ_NAMESPACE_BEGIN

struct OffsetRecord {
  explicit OffsetRecord(int64_t offset) : offset_(offset), released_(false) {}
  OffsetRecord(int64_t offset, bool released) : offset_(offset), released_(released) {}
  int64_t offset_;
  bool released_;
};

ROCKETMQ_NAMESPACE_END

namespace std {

template <> struct less<ROCKETMQ_NAMESPACE::OffsetRecord> {
  bool operator()(const ROCKETMQ_NAMESPACE::OffsetRecord& lhs, const ROCKETMQ_NAMESPACE::OffsetRecord& rhs) const {
    return lhs.offset_ < rhs.offset_;
  }
};

} // namespace std

ROCKETMQ_NAMESPACE_BEGIN

class PushConsumer;

/**
 * @brief Once messages are fetched(either pulled or popped) from remote server, they are firstly put into cache.
 * Dispatcher thread, after waking up, will submit them into thread-pool. Messages at this phase are called "inflight"
 * state. Once messages are processed by user-passed-in callback, their quota will be released for future incoming
 * messages.
 */
class ProcessQueueImpl : virtual public ProcessQueue {
public:
  ProcessQueueImpl(MQMessageQueue message_queue, FilterExpression filter_expression,
                   std::weak_ptr<PushConsumer> consumer, std::shared_ptr<ClientManager> client_instance);

  ~ProcessQueueImpl() override;

  void callback(std::shared_ptr<ReceiveMessageCallback> callback) override;

  MQMessageQueue getMQMessageQueue() override;

  bool expired() const override;

  bool shouldThrottle() const override LOCKS_EXCLUDED(messages_mtx_);

  const FilterExpression& getFilterExpression() const override;

  std::weak_ptr<PushConsumer> getConsumer() override;

  std::shared_ptr<ClientManager> getClientManager() override;

  void receiveMessage() override;

  const std::string& simpleName() const override { return simple_name_; }

  std::string topic() const override { return message_queue_.getTopic(); }

  bool hasPendingMessages() const override LOCKS_EXCLUDED(messages_mtx_);

  /**
   * Put message fetched from broker into cache.
   *
   * @param messages
   */
  void cacheMessages(const std::vector<MQMessageExt>& messages) override LOCKS_EXCLUDED(messages_mtx_, offsets_mtx_);

  /**
   * @return Number of messages that is not yet dispatched to thread pool, likely, due to topic-rate-limiting.
   */
  uint32_t cachedMessagesSize() const LOCKS_EXCLUDED(messages_mtx_) {
    absl::MutexLock lk(&messages_mtx_);
    return cached_messages_.size();
  }

  /**
   * Dispatch messages from cache to thread pool in form of consumeTask.
   * @param batch_size
   * @param messages
   * @return true if there are more messages to consume in cache
   */
  bool take(uint32_t batch_size, std::vector<MQMessageExt>& messages) override LOCKS_EXCLUDED(messages_mtx_);

  void syncIdleState() override { idle_since_ = std::chrono::steady_clock::now(); }

  void nextOffset(int64_t next_offset) override {
    assert(next_offset >= 0);
    next_offset_ = next_offset;
  }

  int64_t nextOffset() const { return next_offset_; }

  bool committedOffset(int64_t& offset) override LOCKS_EXCLUDED(offsets_mtx_);

  void release(uint64_t body_size, int64_t offset) override LOCKS_EXCLUDED(messages_mtx_, offsets_mtx_);

  bool unbindFifoConsumeTask() override {
    bool expected = true;
    return has_fifo_task_bound_.compare_exchange_strong(expected, false, std::memory_order_relaxed);
  }

  bool bindFifoConsumeTask() override {
    bool expected = false;
    return has_fifo_task_bound_.compare_exchange_strong(expected, true, std::memory_order_relaxed);
  }

private:
  MQMessageQueue message_queue_;

  /**
   * Expression used to filter message in the server side.
   */
  const FilterExpression filter_expression_;

  std::chrono::milliseconds invisible_time_;

  std::chrono::steady_clock::time_point idle_since_{std::chrono::steady_clock::now()};

  absl::Time create_timestamp_{absl::Now()};

  std::string simple_name_;

  std::weak_ptr<PushConsumer> consumer_;
  std::shared_ptr<ClientManager> client_manager_;

  std::shared_ptr<ReceiveMessageCallback> receive_callback_;

  /**
   * Messages that are pending to be submitted to thread pool.
   */
  mutable std::vector<MQMessageExt> cached_messages_ GUARDED_BY(messages_mtx_);

  mutable absl::Mutex messages_mtx_;

  /**
   * @brief Quantity of the cached messages.
   *
   */
  std::atomic<uint32_t> cached_message_quantity_;

  /**
   * @brief Total body memory size of the cached messages.
   *
   */
  std::atomic<uint64_t> cached_message_memory_;

  int64_t next_offset_{0};

  /**
   * If this process queue is used in FIFO scenario, this field marks if there is an task in thread pool.
   */
  std::atomic_bool has_fifo_task_bound_{false};

  std::set<OffsetRecord> offsets_ GUARDED_BY(offsets_mtx_);
  absl::Mutex offsets_mtx_;

  void popMessage();
  void wrapPopMessageRequest(absl::flat_hash_map<std::string, std::string>& metadata,
                             rmq::ReceiveMessageRequest& request);

  void pullMessage();
  void wrapPullMessageRequest(absl::flat_hash_map<std::string, std::string>& metadata,
                              rmq::PullMessageRequest& request);

  void wrapFilterExpression(rmq::FilterExpression* filter_expression);

  FRIEND_TEST(ProcessQueueTest, testExpired);
};

ROCKETMQ_NAMESPACE_END