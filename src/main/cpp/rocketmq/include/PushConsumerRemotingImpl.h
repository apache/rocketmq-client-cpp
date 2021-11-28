#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "apache/rocketmq/v1/definition.pb.h"

#include "ClientImpl.h"
#include "MixAll.h"
#include "PushConsumer.h"
#include "rocketmq/ExpressionType.h"
#include "rocketmq/MessageListener.h"
#include "rocketmq/MessageModel.h"
#include "rocketmq/OffsetStore.h"
#include "rocketmq/ProtocolType.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class PushConsumerRemotingImpl : virtual public ClientImpl,
                                 virtual public PushConsumer,
                                 public std::enable_shared_from_this<PushConsumerRemotingImpl> {
public:
  explicit PushConsumerRemotingImpl(absl::string_view group_name) : ClientImpl(group_name) {
    protocolType(ProtocolType::Remoting);
  }

  void start() override LOCKS_EXCLUDED(topic_filter_expression_map_mtx_);

  void shutdown() override;

  absl::optional<FilterExpression> getFilterExpression(const std::string& topic) const override
      LOCKS_EXCLUDED(topic_filter_expression_map_mtx_);

  uint32_t maxCachedMessageQuantity() const override {
    return max_cache_message_quantity_;
  }

  uint64_t maxCachedMessageMemory() const override {
    return max_cache_message_memory_;
  }

  int32_t receiveBatchSize() const override {
    return receive_message_batch_size_;
  }

  std::shared_ptr<ConsumeMessageService> getConsumeMessageService() override {
    return nullptr;
  }

  void iterateProcessQueue(const std::function<void(ProcessQueueSharedPtr)>& cb) override {
  }

  MessageModel messageModel() const override {
    return message_model_;
  }

  void ack(const MQMessageExt& msg, const std::function<void(const std::error_code&)>& callback) override {
  }

  void forwardToDeadLetterQueue(const MQMessageExt& message, const std::function<void(bool)>& cb) override {
  }

  const Executor& customExecutor() const override {
    return custom_executor_;
  }

  uint32_t consumeBatchSize() const override {
    return consume_message_batch_size_;
  }

  int32_t maxDeliveryAttempts() const override {
    return max_delivery_attempts_;
  }

  void updateOffset(const MQMessageQueue& message_queue, int64_t offset) override {
  }

  void nack(const MQMessageExt& message, const std::function<void(const std::error_code&)>& callback) override {
  }

  bool receiveMessage(const MQMessageQueue& message_queue, const FilterExpression& filter_expression) override {
    return true;
  }

  MessageListener* messageListener() override {
    return message_listener_;
  }

  void setOffsetStore(std::unique_ptr<OffsetStore> offset_store) override {
    offset_store_ = std::move(offset_store);
  }

  std::shared_ptr<ClientImpl> self() override {
    return shared_from_this();
  }

  void subscribe(std::string topic, std::string tag) LOCKS_EXCLUDED(topic_filter_expression_map_mtx_);

  void prepareHeartbeatData(HeartbeatRequest& request) override;

private:
  absl::flat_hash_map<std::string, FilterExpression>
      topic_filter_expression_map_ GUARDED_BY(topic_filter_expression_map_mtx_);
  mutable absl::Mutex topic_filter_expression_map_mtx_;

  MessageModel message_model_{MessageModel::CLUSTERING};

  std::uint32_t max_cache_message_quantity_{MixAll::DEFAULT_CACHED_MESSAGE_COUNT};
  std::uint64_t max_cache_message_memory_{MixAll::DEFAULT_CACHED_MESSAGE_MEMORY};
  std::int32_t receive_message_batch_size_{MixAll::DEFAULT_RECEIVE_MESSAGE_BATCH_SIZE};
  std::uint32_t consume_message_batch_size_{MixAll::DEFAULT_CONSUME_MESSAGE_BATCH_SIZE};
  std::int32_t max_delivery_attempts_{MixAll::DEFAULT_MAX_DELIVERY_ATTEMPTS};

  Executor custom_executor_;

  MessageListener* message_listener_;

  std::unique_ptr<OffsetStore> offset_store_;
};

ROCKETMQ_NAMESPACE_END