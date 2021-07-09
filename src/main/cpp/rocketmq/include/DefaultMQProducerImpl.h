#pragma once

#include <chrono>
#include <memory>
#include <mutex>
#include <string>

#include "BaseImpl.h"
#include "ClientConfig.h"
#include "ClientInstance.h"
#include "MixAll.h"
#include "TopicPublishInfo.h"
#include "TransactionImpl.h"
#include "rocketmq/AsyncCallback.h"
#include "rocketmq/LocalTransactionStateChecker.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQSelector.h"
#include "rocketmq/SendResult.h"
#include "rocketmq/State.h"

ROCKETMQ_NAMESPACE_BEGIN

class DefaultMQProducerImpl : public BaseImpl, public std::enable_shared_from_this<DefaultMQProducerImpl> {
public:
  explicit DefaultMQProducerImpl(std::string group_name);

  ~DefaultMQProducerImpl() override;

  void prepareHeartbeatData(HeartbeatRequest& request) override;

  void start() override;

  void shutdown() override;

  SendResult send(const MQMessage& message);
  SendResult send(const MQMessage& message, const MQMessageQueue& message_queue);
  SendResult send(const MQMessage& message, MessageQueueSelector* selector, void* arg);

  SendResult send(const MQMessage& message, MessageQueueSelector* selector, void* arg, int auto_retry_times);

  void send(const MQMessage& message, SendCallback* callback);
  void send(const MQMessage& message, const MQMessageQueue& message_queue, SendCallback* callback);
  void send(const MQMessage& message, MessageQueueSelector* selector, void* arg, SendCallback* callback);

  void sendOneway(const MQMessage& message);
  void sendOneway(const MQMessage& message, const MQMessageQueue& message_queue);
  void sendOneway(const MQMessage& message, MessageQueueSelector* selector, void* arg);

  void setLocalTransactionStateChecker(LocalTransactionStateCheckerPtr checker);

  std::unique_ptr<TransactionImpl> prepare(const MQMessage& message);

  bool commit(const std::string& message_id, const std::string& transaction_id, const std::string& target);

  bool rollback(const std::string& message_id, const std::string& transaction_id, const std::string& target);

  /**
   * Check if the RPC client for the target host is isolated or not
   * @param endpoint Address of target host.
   * @return true if client is active; false otherwise.
   */
  bool isEndpointIsolated(const std::string& endpoint) LOCKS_EXCLUDED(isolated_endpoints_mtx_);

  /**
   * Note: This function is purpose-made public such that the whole isolate/add-back mechanism can be properly tested.
   * @param target Endpoint of the target host
   */
  void isolateEndpoint(const std::string& target) LOCKS_EXCLUDED(isolated_endpoints_mtx_);

  int maxAttemptTimes() const { return max_attempt_times_; }

  void maxAttemptTimes(int times) { max_attempt_times_ = times; }

  int getFailedTimes() const { return failed_times_; }

  void setFailedTimes(int times) { failed_times_ = times; }

  std::vector<MQMessageQueue> getTopicMessageQueueInfo(const std::string& topic);

  uint32_t compressBodyThreshold() const { return compress_body_threshold_; }

  void compressBodyThreshold(uint32_t threshold) { compress_body_threshold_ = threshold; }

protected:
  std::shared_ptr<BaseImpl> self() override { return shared_from_this(); }

  void resolveOrphanedTransactionalMessage(const std::string& transaction_id, const MQMessageExt& message) override;

private:
  absl::flat_hash_map<std::string, TopicPublishInfoPtr> topic_publish_info_table_ GUARDED_BY(topic_publish_info_mtx_);
  absl::Mutex topic_publish_info_mtx_; // protects topic_publish_info_

  int32_t max_attempt_times_{MixAll::MAX_SEND_MESSAGE_ATTEMPT_TIMES_};
  int32_t failed_times_{0}; // only for test
  uint32_t compress_body_threshold_;

  LocalTransactionStateCheckerPtr transaction_state_checker_;

  void asyncPublishInfo(const std::string& topic, const std::function<void(const TopicPublishInfoPtr&)>& cb)
      LOCKS_EXCLUDED(topic_publish_info_mtx_);

  TopicPublishInfoPtr getPublishInfo(const std::string& topic);

  bool executeMessageQueueSelector(const MQMessage& message, MessageQueueSelector* selector, void* arg,
                                   std::vector<MQMessageQueue>& result);

  void takeMessageQueuesRoundRobin(const TopicPublishInfoPtr& publish_info, std::vector<MQMessageQueue>& message_queues,
                                   int number);

  std::string wrapSendMessageRequest(const MQMessage& message, SendMessageRequest& request,
                                     const MQMessageQueue& message_queue);

  bool isRunning() const;

  void ensureRunning() const;

  void validate(const MQMessage& message);

  void send0(const MQMessage& message, SendCallback* callback, std::vector<MQMessageQueue> list, int max_attempt_times);

  bool endTransaction0(const std::string& target, const std::string& message_id, const std::string& transaction_id,
                       TransactionState resolution);

  void isolatedEndpoints(absl::flat_hash_set<std::string>& endpoints) LOCKS_EXCLUDED(isolated_endpoints_mtx_);
};

ROCKETMQ_NAMESPACE_END