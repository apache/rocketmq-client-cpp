#pragma once

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <memory>

#include "ClientInstance.h"
#include "FilterExpression.h"
#include "MixAll.h"
#include "ReceiveMessageCallback.h"
#include "TopicAssignmentInfo.h"
#include "absl/container/flat_hash_map.h"
#include "apache/rocketmq/v1/service.pb.h"
#include "rocketmq/ConsumerType.h"
#include "rocketmq/MQMessageExt.h"
#include "rocketmq/MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

class DefaultMQPushConsumerImpl;
class ProcessQueue {
public:
  ProcessQueue(MQMessageQueue message_queue, FilterExpression filter_expression, ConsumeMessageType consume_type,
               int max_cache_size, std::weak_ptr<DefaultMQPushConsumerImpl> call_back_owner,
               std::shared_ptr<ClientInstance> client_instance);

  ~ProcessQueue();

  void callback(std::shared_ptr<ReceiveMessageCallback> callback);

  MQMessageQueue getMQMessageQueue();

  bool expired() const;

  bool shouldThrottle() const;

  const FilterExpression& getFilterExpression() const;

  std::weak_ptr<DefaultMQPushConsumerImpl> getCallbackOwner();

  std::shared_ptr<ClientInstance> getClientInstance();

  void receiveMessage();

  const std::string& simpleName() const { return simple_name_; }

  std::string topic() const { return message_queue_.getTopic(); }

  /**
   * Put message fetched from broker into cache.
   *
   * @param messages
   */
  void cacheMessages(const std::vector<MQMessageExt>& messages) LOCKS_EXCLUDED(cached_messages_mtx_);

  /**
   * @return Number of messages that is not yet dispatched to thread pool, likely, due to topic-rate-limiting.
   */
  uint32_t cachedMessagesSize() const LOCKS_EXCLUDED(cached_messages_mtx_) {
    absl::MutexLock lk(&cached_messages_mtx_);
    return cached_messages_.size();
  }

  /**
   * Dispatch messages from cache to thread pool in form of consumeTask.
   * @param batch_size
   * @param messages
   * @return true if there are more messages to consume in cache
   */
  bool consume(int batch_size, std::vector<MQMessageExt>& messages) LOCKS_EXCLUDED(cached_messages_mtx_);

  void updateThrottleTimestamp() { last_throttle_timestamp_ = std::chrono::steady_clock::now(); }

  std::atomic_int& messageCachedNumber() { return message_cached_number_; }

  ConsumeMessageType consumeType() const { return consume_type_; }

  void nextOffset(int64_t next_offset) {
    assert(next_offset >= 0);
    next_offset_ = next_offset;
  }

  int64_t nextOffset() const { return next_offset_; }

private:
  MQMessageQueue message_queue_;

  /**
   * Expression used to filter message in the server side.
   */
  const FilterExpression filter_expression_;

  ConsumeMessageType consume_type_{ConsumeMessageType::POP};

  int max_message_number_;
  std::chrono::milliseconds invisible_time_;

  std::chrono::steady_clock::time_point last_poll_timestamp_{std::chrono::steady_clock::now()};

  std::chrono::steady_clock::time_point last_throttle_timestamp_{std::chrono::steady_clock::now()};

  absl::Time born_timestamp_{absl::Now()};

  ConsumeInitialMode initial_mode_{ConsumeInitialMode::MAX};

  /**
   * Number of messages that are not yet acknowledged.
   */
  std::atomic_int message_cached_number_;

  /**
   * Maximum number of locally cached messages. Once exceeding this threshold, fetch-loop should be throttled.
   */
  int max_cache_size_;

  std::string simple_name_;

  // callback
  std::weak_ptr<DefaultMQPushConsumerImpl> call_back_owner_;
  std::shared_ptr<ClientInstance> client_instance_;

  std::shared_ptr<ReceiveMessageCallback> callback_;

  mutable std::vector<MQMessageExt> cached_messages_;
  mutable absl::Mutex cached_messages_mtx_;

  int64_t next_offset_{-1};

  void popMessage();
  void wrapPopMessageRequest(absl::flat_hash_map<std::string, std::string>& metadata,
                             rmq::ReceiveMessageRequest& request);

  void pullMessage();
  void wrapPullMessageRequest(absl::flat_hash_map<std::string, std::string>& metadata,
                              rmq::PullMessageRequest& request);
};

using ProcessQueueSharedPtr = std::shared_ptr<ProcessQueue>;
using ProcessQueueWeakPtr = std::weak_ptr<ProcessQueue>;

ROCKETMQ_NAMESPACE_END