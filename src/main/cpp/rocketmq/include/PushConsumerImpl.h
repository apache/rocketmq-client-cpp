#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <system_error>

#include "absl/strings/string_view.h"

#include "ClientConfigImpl.h"
#include "ClientImpl.h"
#include "ClientManagerImpl.h"
#include "ConsumeMessageService.h"
#include "FilterExpression.h"
#include "ProcessQueue.h"
#include "PushConsumer.h"
#include "Scheduler.h"
#include "TopicAssignmentInfo.h"
#include "TopicPublishInfo.h"
#include "UtilAll.h"
#include "apache/rocketmq/v1/service.pb.h"
#include "rocketmq/DefaultMQPushConsumer.h"
#include "rocketmq/OffsetStore.h"
#include "rocketmq/State.h"

ROCKETMQ_NAMESPACE_BEGIN

class ConsumeMessageService;
class ConsumeFifoMessageService;
class ConsumeStandardMessageService;

class PushConsumerImpl : virtual public ClientImpl,
                         virtual public PushConsumer,
                         public std::enable_shared_from_this<PushConsumerImpl> {
public:
  explicit PushConsumerImpl(absl::string_view group_name);

  ~PushConsumerImpl() override;

  void prepareHeartbeatData(HeartbeatRequest& request) override LOCKS_EXCLUDED(topic_filter_expression_table_mtx_);

  void start() override;

  void shutdown() override;

  void subscribe(const std::string& topic, const std::string& expression,
                 ExpressionType expression_type = ExpressionType::TAG)
      LOCKS_EXCLUDED(topic_filter_expression_table_mtx_);

  void unsubscribe(const std::string& topic) LOCKS_EXCLUDED(topic_filter_expression_table_mtx_);

  absl::optional<FilterExpression> getFilterExpression(const std::string& topic) const override
      LOCKS_EXCLUDED(topic_filter_expression_table_mtx_);

  void setConsumeFromWhere(ConsumeFromWhere consume_from_where);

  void registerMessageListener(MessageListener* message_listener);

  void scanAssignments() LOCKS_EXCLUDED(topic_filter_expression_table_mtx_);

  static bool selectBroker(const TopicRouteDataPtr& route, std::string& broker_host);

  void wrapQueryAssignmentRequest(const std::string& topic, const std::string& consumer_group,
                                  const std::string& client_id, const std::string& strategy_name,
                                  QueryAssignmentRequest& request);

  /**
   * Query assignment of the specified topic from load balancer directly if
   * message consuming mode is clustering. In case current client is operating
   * in the broadcasting mode, assignments are constructed locally from topic
   * route entries.
   *
   * @param topic Topic to query
   * @return shared pointer to topic assignment info
   */
  void queryAssignment(const std::string& topic,
                       const std::function<void(const std::error_code&, const TopicAssignmentPtr&)>& cb);

  void syncProcessQueue(const std::string& topic, const TopicAssignmentPtr& topic_assignment,
                        const FilterExpression& filter_expression) LOCKS_EXCLUDED(process_queue_table_mtx_);

  ProcessQueueSharedPtr getOrCreateProcessQueue(const MQMessageQueue& message_queue,
                                                const FilterExpression& filter_expression)
      LOCKS_EXCLUDED(process_queue_table_mtx_);

  bool receiveMessage(const MQMessageQueue& message_queue, const FilterExpression& filter_expression) override
      LOCKS_EXCLUDED(process_queue_table_mtx_);

  uint32_t consumeThreadPoolSize() const;

  void consumeThreadPoolSize(int thread_pool_size);

  int32_t maxDeliveryAttempts() const override { return max_delivery_attempts_; }

  uint32_t consumeBatchSize() const override;

  void consumeBatchSize(uint32_t consume_batch_size);

  int32_t receiveBatchSize() const override { return receive_batch_size_; }

  std::shared_ptr<ConsumeMessageService> getConsumeMessageService() override;

  void ack(const MQMessageExt& msg, const std::function<void(const std::error_code&)>& callback) override;

  /**
   * Negative acknowledge the given message; Refer to
   * https://en.wikipedia.org/wiki/Acknowledgement_(data_networks) for
   * background info.
   *
   * Current implementation is to change invisible time of the given message.
   *
   * @param message Message to negate on the broker side.
   */
  void nack(const MQMessageExt& message, const std::function<void(const std::error_code&)>& callback) override;

  void forwardToDeadLetterQueue(const MQMessageExt& message, const std::function<void(bool)>& cb) override;

  void wrapAckMessageRequest(const MQMessageExt& msg, AckMessageRequest& request);

  // only for test
  std::size_t getProcessQueueTableSize() LOCKS_EXCLUDED(process_queue_table_mtx_);

  void setCustomExecutor(const Executor& executor) { custom_executor_ = executor; }

  const Executor& customExecutor() const override { return custom_executor_; }

  void setThrottle(const std::string& topic, uint32_t threshold);

#ifdef ENABLE_TRACING
  nostd::shared_ptr<trace::Tracer> getTracer();
#endif

  MessageModel messageModel() const override { return message_model_; }

  void setMessageModel(MessageModel message_model) { message_model_ = message_model; }

  void offsetStore(std::unique_ptr<OffsetStore> offset_store) { offset_store_ = std::move(offset_store); }

  void updateOffset(const MQMessageQueue& message_queue, int64_t offset) override {
    if (offset_store_) {
      offset_store_->updateOffset(message_queue, offset);
    }
  }

  /**
   * Max number of messages that may be cached per queue before applying
   * back-pressure.
   * @return
   */
  uint32_t maxCachedMessageQuantity() const override { return MixAll::DEFAULT_CACHED_MESSAGE_COUNT; }

  /**
   * Threshold of total cached message body size by queue before applying
   * back-pressure.
   * @return
   */
  uint64_t maxCachedMessageMemory() const override { return MixAll::DEFAULT_CACHED_MESSAGE_MEMORY; }

  void iterateProcessQueue(const std::function<void(ProcessQueueSharedPtr)>& callback) override;

  MessageListener* messageListener() override { return message_listener_; }

  ReceiveMessageAction receiveMessageAction() const override { return receive_message_policy_; }

protected:
  std::shared_ptr<ClientImpl> self() override { return shared_from_this(); }

  ClientResourceBundle resourceBundle() LOCKS_EXCLUDED(topic_filter_expression_table_mtx_) override;

private:
  absl::flat_hash_map<std::string, FilterExpression>
      topic_filter_expression_table_ GUARDED_BY(topic_filter_expression_table_mtx_);
  mutable absl::Mutex topic_filter_expression_table_mtx_;

  /**
   * Consume message thread pool size.
   */
  uint32_t consume_thread_pool_size_{MixAll::DEFAULT_CONSUME_THREAD_POOL_SIZE};

  MessageListener* message_listener_{nullptr};

  std::shared_ptr<ConsumeMessageService> consume_message_service_;
  uint32_t consume_batch_size_{MixAll::DEFAULT_CONSUME_MESSAGE_BATCH_SIZE};

  int32_t receive_batch_size_{MixAll::DEFAULT_RECEIVE_MESSAGE_BATCH_SIZE};

  std::uintptr_t scan_assignment_handle_{0};
  static const char* SCAN_ASSIGNMENT_TASK_NAME;

  absl::flat_hash_map<MQMessageQueue, ProcessQueueSharedPtr> process_queue_table_ GUARDED_BY(process_queue_table_mtx_);
  absl::Mutex process_queue_table_mtx_;

  ConsumeFromWhere consume_from_where_{ConsumeFromWhere::CONSUME_FROM_LAST_OFFSET};
  Executor custom_executor_;

  absl::flat_hash_map<std::string /* Topic */, uint32_t /* Threshold */>
      throttle_table_ GUARDED_BY(throttle_table_mtx_);
  absl::Mutex throttle_table_mtx_;

  int32_t max_delivery_attempts_{MixAll::DEFAULT_MAX_DELIVERY_ATTEMPTS};

  MessageModel message_model_{MessageModel::CLUSTERING};

  ReceiveMessageAction receive_message_policy_{ReceiveMessageAction::POLLING};

  mutable std::unique_ptr<OffsetStore> offset_store_;

  void fetchRoutes() LOCKS_EXCLUDED(topic_filter_expression_table_mtx_);

  friend class ConsumeMessageService;
  friend class ConsumeFifoMessageService;
  friend class ConsumeStandardMessageService;
};

ROCKETMQ_NAMESPACE_END