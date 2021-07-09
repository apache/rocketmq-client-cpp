#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "BaseImpl.h"
#include "ClientConfig.h"
#include "ClientInstance.h"
#include "ConsumeMessageService.h"
#include "FilterExpression.h"
#include "ProcessQueue.h"
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
class ConsumeMessageOrderlyService;
class ConsumeMessageConcurrentlyService;

class DefaultMQPushConsumerImpl : public BaseImpl, public std::enable_shared_from_this<DefaultMQPushConsumerImpl> {
public:
  explicit DefaultMQPushConsumerImpl(std::string group_name);

  ~DefaultMQPushConsumerImpl() override;

  void prepareHeartbeatData(HeartbeatRequest& request) override;

  void start() override;

  void shutdown() override;

  void subscribe(const std::string& topic, const std::string& expression,
                 ExpressionType expression_type = ExpressionType::TAG)
      LOCKS_EXCLUDED(topic_filter_expression_table_mtx_);

  void unsubscribe(const std::string& topic) LOCKS_EXCLUDED(topic_filter_expression_table_mtx_);

  absl::flat_hash_map<std::string, FilterExpression> getTopicFilterExpressionTable()
      LOCKS_EXCLUDED(topic_filter_expression_table_mtx_);

  void setConsumeFromWhere(ConsumeFromWhere consume_from_where);

  void registerMessageListener(MQMessageListener* message_listener);

  void scanAssignments() LOCKS_EXCLUDED(process_queue_table_mtx_);

  static bool selectBroker(const TopicRouteDataPtr& route, std::string& broker_host);

  void wrapQueryAssignmentRequest(const std::string& topic, const std::string& consumer_group,
                                  const std::string& client_id, const std::string& strategy_name,
                                  QueryAssignmentRequest& request);

  /**
   * Query assignment of the specified topic from load balancer directly if message consuming mode is clustering. In
   * case current client is operating in the broadcasting mode, assignments are constructed locally from topic route
   * entries.
   *
   * @param topic Topic to query
   * @return shared pointer to topic assignment info
   */
  void queryAssignment(const std::string& topic, const std::function<void(const TopicAssignmentPtr&)>& cb);

  void syncProcessQueue(const std::string& topic, const TopicAssignmentPtr& topic_assignment,
                        const FilterExpression& filter_expression) LOCKS_EXCLUDED(process_queue_table_mtx_);

  ProcessQueueSharedPtr getOrCreateProcessQueue(const MQMessageQueue& message_queue,
                                                const FilterExpression& filter_expression,
                                                ConsumeMessageType consume_type)
      LOCKS_EXCLUDED(process_queue_table_mtx_);

  bool receiveMessage(const MQMessageQueue& message_queue, const FilterExpression& filter_expression,
                      ConsumeMessageType consume_type) LOCKS_EXCLUDED(process_queue_table_mtx_);

  int consumeThreadPoolSize() const;

  void consumeThreadPoolSize(int thread_pool_size);

  uint32_t consumeBatchSize() const;

  int32_t receiveBatchSize() const { return receive_message_batch_size_; }

  void consumeBatchSize(uint32_t consume_batch_size);

  void maxCachedMessageNumberPerQueue(int max_cached_message_number_per_queue);

  std::shared_ptr<ConsumeMessageService> getConsumeMessageService();

  void ack(const MQMessageExt& msg, const std::function<void(bool)>& callback);

  /**
   * Negative acknowledge the given message; Refer to https://en.wikipedia.org/wiki/Acknowledgement_(data_networks) for
   * background info.
   *
   * Current implementation is to change invisible time of the given message.
   *
   * @param message Message to negate on the broker side.
   */
  void nack(const MQMessageExt& message, const std::function<void(bool)>& callback);

  void wrapAckMessageRequest(const MQMessageExt& msg, AckMessageRequest& request);

  bool isStopped() const;

  // only for test
  int getProcessQueueTableSize() LOCKS_EXCLUDED(process_queue_table_mtx_);

  void setCustomExecutor(const Executor& executor) { custom_executor_ = executor; }

  const Executor& customExecutor() const { return custom_executor_; }

  void setThrottle(const std::string& topic, uint32_t threshold);

#ifdef ENABLE_TRACING
  nostd::shared_ptr<trace::Tracer> getTracer();
#endif

  MessageModel messageModel() const { return message_model_; }

  void setMessageModel(MessageModel message_model) { message_model_ = message_model; }

  void offsetStore(std::unique_ptr<OffsetStore> offset_store) { offset_store_ = std::move(offset_store); }

protected:
  std::shared_ptr<BaseImpl> self() override { return shared_from_this(); }

  ClientResourceBundle resourceBundle() LOCKS_EXCLUDED(topic_filter_expression_table_mtx_) override;

private:
  absl::flat_hash_map<std::string, FilterExpression>
      topic_filter_expression_table_ GUARDED_BY(topic_filter_expression_table_mtx_);
  absl::Mutex topic_filter_expression_table_mtx_;

  /**
   * Consume message thread pool size.
   */
  int consume_thread_pool_size_;

  MQMessageListener* message_listener_ptr_;

  std::shared_ptr<ConsumeMessageService> consume_message_service_;

  uint32_t consume_batch_size_;

  // TODO: make it configurable
  int32_t receive_message_batch_size_{32};

  int max_cached_message_number_per_queue_;

  std::uintptr_t scan_assignment_handle_{0};
  static const char* SCAN_ASSIGNMENT_TASK_NAME;

  absl::flat_hash_map<MQMessageQueue, ProcessQueueSharedPtr> process_queue_table_ GUARDED_BY(process_queue_table_mtx_);
  absl::Mutex process_queue_table_mtx_;

  ConsumeFromWhere consume_from_where_{ConsumeFromWhere::CONSUME_FROM_LAST_OFFSET};
  Executor custom_executor_;

  absl::flat_hash_map<std::string /* Topic */, uint32_t /* Threshold */>
      throttle_table_ GUARDED_BY(throttle_table_mtx_);
  absl::Mutex throttle_table_mtx_;

  // TODO: Make default value configurable
  int32_t max_delivery_attempts_{16};

  MessageModel message_model_{MessageModel::CLUSTERING};

  std::unique_ptr<OffsetStore> offset_store_;

  void fetchRoutes() LOCKS_EXCLUDED(topic_filter_expression_table_mtx_);

  void iterateProcessQueue(const std::function<void(ProcessQueueSharedPtr)>& functor);

  friend class ConsumeMessageConcurrentlyService;
  friend class ConsumeMessageOrderlyService;

  static const int32_t MAX_CACHED_MESSAGE_COUNT;
  static const int32_t DEFAULT_CACHED_MESSAGE_COUNT;
  static const int32_t DEFAULT_CONSUME_MESSAGE_BATCH_SIZE;
  static const int32_t DEFAULT_CONSUME_THREAD_POOL_SIZE;
};

class AsyncReceiveMessageCallback : public ReceiveMessageCallback,
                                    public std::enable_shared_from_this<AsyncReceiveMessageCallback> {
public:
  explicit AsyncReceiveMessageCallback(ProcessQueueWeakPtr process_queue);

  ~AsyncReceiveMessageCallback() override = default;

  void onSuccess(ReceiveMessageResult& result) override;

  void onException(MQException& e) override;

  void receiveMessageLater();

  void receiveMessageImmediately();

private:
  /**
   * Hold a weak_ptr to ProcessQueue. Once ProcessQueue was released, stop the pop-cycle immediately.
   */
  ProcessQueueWeakPtr process_queue_;

  std::function<void(void)> receive_message_later_;

  void checkThrottleThenReceive();

  static const char* RECEIVE_LATER_TASK_NAME;
};

ROCKETMQ_NAMESPACE_END