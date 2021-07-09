#include "DefaultMQProducerImpl.h"
#include "MessageAccessor.h"
#include "Metadata.h"
#include "MixAll.h"
#include "UtilAll.h"
#include "Protocol.h"
#include "SendCallbacks.h"
#include "SendMessageContext.h"
#include "Signature.h"
#include "TransactionImpl.h"
#include "UniqueIdGenerator.h"
#include "rocketmq/ErrorCode.h"
#include "rocketmq/MQClientException.h"

ROCKETMQ_NAMESPACE_BEGIN

DefaultMQProducerImpl::DefaultMQProducerImpl(std::string group_name)
    : BaseImpl(std::move(group_name)), compress_body_threshold_(MixAll::DEFAULT_COMPRESS_BODY_THRESHOLD_) {
  // TODO: initialize client_config_ and fault_strategy_
}

DefaultMQProducerImpl::~DefaultMQProducerImpl() { SPDLOG_INFO("Producer instance is destructed"); }

void DefaultMQProducerImpl::start() {
  BaseImpl::start();
  if (State::STARTED != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected producer state: {}", state_.load(std::memory_order_relaxed));
    return;
  }

  client_instance_->addClientObserver(shared_from_this());
}

void DefaultMQProducerImpl::shutdown() {
  BaseImpl::shutdown();

  State expected = State::STOPPING;
  if (state_.compare_exchange_strong(expected, State::STOPPED)) {
    SPDLOG_INFO("DefaultMQProducerImpl stopped");
  }
}

bool DefaultMQProducerImpl::isRunning() const { return State::STARTED == state_.load(std::memory_order_relaxed); }

void DefaultMQProducerImpl::ensureRunning() const {
  if (!isRunning()) {
    THROW_MQ_EXCEPTION(MQClientException, "Invoke #start() first", ILLEGAL_STATE);
  }
}

void DefaultMQProducerImpl::validate(const MQMessage& message) {}

std::string DefaultMQProducerImpl::wrapSendMessageRequest(const MQMessage& message, SendMessageRequest& request,
                                                          const MQMessageQueue& message_queue) {
  request.mutable_message()->mutable_topic()->set_arn(arn_);
  request.mutable_message()->mutable_topic()->set_name(message.getTopic());

  auto system_attribute = request.mutable_message()->mutable_system_attribute();

  // Born-time
  auto duration = absl::Now() - absl::UnixEpoch();
  int64_t seconds = absl::ToInt64Seconds(duration);
  system_attribute->mutable_born_timestamp()->set_seconds(seconds);
  system_attribute->mutable_born_timestamp()->set_nanos(absl::ToInt64Nanoseconds(duration - absl::Seconds(seconds)));

  system_attribute->set_born_host(UtilAll::hostname());

  system_attribute->mutable_producer_group()->set_arn(arn_);
  system_attribute->mutable_producer_group()->set_name(group_name_);

  // Set system flag if the message is transactional
  const auto& properties = message.getProperties();
  auto search = properties.find(MixAll::PROPERTY_TRANSACTION_PREPARED_);
  // TODO: set message type for normal/order/delay
  if (search != properties.end()) {
    const std::string& transactional_flag = search->second;
    if ("true" == transactional_flag) {
      system_attribute->set_message_type(rmq::MessageType::TRANSACTION);
    }
  }

  if (message.bodyLength() >= compress_body_threshold_) {
    std::string compressed_body;
    UtilAll::compress(message.getBody(), compressed_body);
    request.mutable_message()->set_body(compressed_body);
    system_attribute->set_body_encoding(rmq::Encoding::GZIP);
  } else {
    request.mutable_message()->set_body(message.getBody());
    system_attribute->set_body_encoding(rmq::Encoding::IDENTITY);
  }

  for (auto& item : message.getProperties()) {
    request.mutable_message()->mutable_user_attribute()->insert({item.first, item.second});
  }

  // Create unique message-id
  std::string message_id = UniqueIdGenerator::instance().next();
  system_attribute->set_message_id(message_id);
  system_attribute->set_partition_id(message_queue.getQueueId());

  SPDLOG_DEBUG("SendMessageRequest: {}", request.DebugString());
  return message_id;
}

SendResult DefaultMQProducerImpl::send(const MQMessage& message) {
  ensureRunning();
  auto topic_publish_info = getPublishInfo(message.getTopic());
  if (!topic_publish_info) {
    THROW_MQ_EXCEPTION(MQClientException, "No topic route available", NO_TOPIC_ROUTE_INFO);
  }

  std::vector<MQMessageQueue> message_queue_list;
  takeMessageQueuesRoundRobin(topic_publish_info, message_queue_list, max_attempt_times_);
  if (message_queue_list.empty()) {
    THROW_MQ_EXCEPTION(MQClientException, "No topic route available", NO_TOPIC_ROUTE_INFO);
  }

  AwaitSendCallback callback;
  send0(message, &callback, message_queue_list, max_attempt_times_);
  callback.await();

  if (callback) {
    return callback.sendResult();
  }
  THROW_MQ_EXCEPTION(MQClientException, callback.errorMessage(), FAILED_TO_SEND_MESSAGE);
}

SendResult DefaultMQProducerImpl::send(const MQMessage& message, const MQMessageQueue& message_queue) {
  ensureRunning();
  std::vector<MQMessageQueue> message_queue_list{message_queue};
  AwaitSendCallback callback;
  send0(message, &callback, message_queue_list, max_attempt_times_);
  callback.await();
  if (callback) {
    return callback.sendResult();
  }
  THROW_MQ_EXCEPTION(MQClientException, callback.errorMessage(), FAILED_TO_SEND_MESSAGE);
}

SendResult DefaultMQProducerImpl::send(const MQMessage& message, MessageQueueSelector* selector, void* arg) {
  ensureRunning();
  std::vector<MQMessageQueue> message_queue_list;
  executeMessageQueueSelector(message, selector, arg, message_queue_list);
  if (message_queue_list.empty()) {
    THROW_MQ_EXCEPTION(MQClientException, "No topic route available", NO_TOPIC_ROUTE_INFO);
  }
  AwaitSendCallback callback;
  send0(message, &callback, message_queue_list, max_attempt_times_);
  callback.await();
  if (callback) {
    return callback.sendResult();
  }
  THROW_MQ_EXCEPTION(MQClientException, callback.errorMessage(), FAILED_TO_SEND_MESSAGE);
}

SendResult DefaultMQProducerImpl::send(const MQMessage& message, MessageQueueSelector* selector, void* arg,
                                       int max_attempt_times) {
  ensureRunning();
  std::vector<MQMessageQueue> message_queue_list;
  executeMessageQueueSelector(message, selector, arg, message_queue_list);
  if (message_queue_list.empty()) {
    THROW_MQ_EXCEPTION(MQClientException, "No topic route available", NO_TOPIC_ROUTE_INFO);
  }
  AwaitSendCallback callback;
  send0(message, &callback, message_queue_list, max_attempt_times);
  callback.await();
  if (callback) {
    return callback.sendResult();
  }
  THROW_MQ_EXCEPTION(MQClientException, callback.errorMessage(), FAILED_TO_SEND_MESSAGE);
}

void DefaultMQProducerImpl::send(const MQMessage& message, SendCallback* cb) {
  ensureRunning();
  auto callback = [this, message, cb](const TopicPublishInfoPtr& publish_info) {
    if (!publish_info) {
      MQClientException e("Failed to acquire topic route data", NO_TOPIC_ROUTE_INFO, __FILE__, __LINE__);
      cb->onException(e);
      return;
    }

    std::vector<MQMessageQueue> message_queue_list;
    takeMessageQueuesRoundRobin(publish_info, message_queue_list, max_attempt_times_);
    send0(message, cb, message_queue_list, max_attempt_times_);
  };

  asyncPublishInfo(message.getTopic(), callback);
}

void DefaultMQProducerImpl::send(const MQMessage& message, const MQMessageQueue& message_queue,
                                 SendCallback* callback) {
  ensureRunning();
  std::vector<MQMessageQueue> message_queue_list{message_queue};
  send0(message, callback, message_queue_list, max_attempt_times_);
}

void DefaultMQProducerImpl::send(const MQMessage& message, MessageQueueSelector* selector, void* arg,
                                 SendCallback* callback) {
  ensureRunning();

  auto cb = [this, message, selector, callback, arg](const TopicPublishInfoPtr& ptr) {
    if (!ptr) {
      MQClientException e("Failed to acquire topic route", NO_TOPIC_ROUTE_INFO, __FILE__, __LINE__);
      callback->onException(e);
      return;
    }

    MQMessageQueue queue = selector->select(ptr->getMessageQueueList(), message, arg);
    std::vector<MQMessageQueue> message_queue_list{queue};

    send0(message, callback, message_queue_list, max_attempt_times_);
  };

  asyncPublishInfo(message.getTopic(), cb);
}

void DefaultMQProducerImpl::sendOneway(const MQMessage& message) {
  ensureRunning();
  auto callback = [this, message](const TopicPublishInfoPtr& ptr) {
    if (!ptr) {
      SPDLOG_WARN("Failed acquire topic publish info for {}", message.getTopic());
      return;
    }
    MQMessageQueue message_queue;
    absl::flat_hash_set<std::string> isolated;
    isolatedEndpoints(isolated);
    ptr->selectOneActiveMessageQueue(isolated, message_queue);
    std::vector<MQMessageQueue> list{message_queue};
    send0(message, onewaySendCallback(), list, 1);
  };
  asyncPublishInfo(message.getTopic(), callback);
}

void DefaultMQProducerImpl::sendOneway(const MQMessage& message, const MQMessageQueue& message_queue) {
  ensureRunning();
  std::vector<MQMessageQueue> list{message_queue};
  send0(message, onewaySendCallback(), list, 1);
}

void DefaultMQProducerImpl::sendOneway(const MQMessage& message, MessageQueueSelector* selector, void* arg) {
  ensureRunning();

  auto callback = [this, message, selector, arg](const TopicPublishInfoPtr& ptr) {
    if (!ptr) {
      SPDLOG_WARN("No topic route for {}", message.getTopic());
      return;
    }
    MQMessageQueue queue = selector->select(ptr->getMessageQueueList(), message, arg);
    std::vector<MQMessageQueue> list{queue};
    send0(message, onewaySendCallback(), list, 1);
  };

  asyncPublishInfo(message.getTopic(), callback);
}

void DefaultMQProducerImpl::setLocalTransactionStateChecker(LocalTransactionStateCheckerPtr checker) {
  transaction_state_checker_ = std::move(checker);
}

void DefaultMQProducerImpl::send0(const MQMessage& message, SendCallback* callback, std::vector<MQMessageQueue> list,
                                  int max_attempt_times) {
  assert(callback);
  if (list.empty()) {
    MQClientException e("Topic route not found", -1, __FILE__, __LINE__);
    callback->onException(e);
    return;
  }

  if (max_attempt_times <= 0) {
    MQClientException e("Retry times illegal", BAD_CONFIGURATION, __FILE__, __LINE__);
    callback->onException(e);
    return;
  }

  const std::string& target = list[0].serviceAddress();
  if (target.empty()) {
    SPDLOG_DEBUG("Unable to resolve broker address");
    THROW_MQ_EXCEPTION(MQClientException, "Failed to resolve broker address from topic route",
                       FAILED_TO_RESOLVE_BROKER_ADDRESS_FROM_TOPIC_ROUTE);
  }

  SendMessageRequest request;
  // Unique message-id is generated using IP, timestamp, hash-code and sequence during the transformation
  wrapSendMessageRequest(message, request, list[0]);

  absl::flat_hash_map<std::string, std::string> metadata;

  // Sign the request. Metadata entries will be part of HTT2 headers frame
  Signature::sign(this, metadata);

  if (max_attempt_times == 1) {
    client_instance_->send(target, metadata, request, callback);
    return;
  }

  auto retry_callback =
      new RetrySendCallback(client_instance_, metadata, request, max_attempt_times, callback, std::move(list));
  client_instance_->send(target, metadata, request, retry_callback);
}

bool DefaultMQProducerImpl::endTransaction0(const std::string& target, const std::string& message_id,
                                            const std::string& transaction_id, TransactionState resolution) {

  EndTransactionRequest request;
  request.set_message_id(message_id);
  request.set_transaction_id(transaction_id);
  std::string action;
  switch (resolution) {
  case TransactionState::COMMIT:
    request.set_resolution(rmq::EndTransactionRequest_TransactionResolution_COMMIT);
    action = "commit";
    break;
  case TransactionState::ROLLBACK:
    request.set_resolution(rmq::EndTransactionRequest_TransactionResolution_ROLLBACK);
    action = "rollback";
    break;
  }
  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(this, metadata);
  bool completed = false;
  bool success = false;
  absl::Mutex mtx;
  absl::CondVar cv;
  auto cb = [&](bool rpc_ok, const EndTransactionResponse& response) {
    completed = true;
    if (!rpc_ok) {
      SPDLOG_WARN("Failed to send {} transaction request to {}", action, target);
      success = false;
    } else if (response.common().status().code() != google::rpc::Code::OK) {
      success = false;
      SPDLOG_WARN("Server[host={}] failed to {} transaction. Reason: {}", target, action,
                  response.common().DebugString());
    } else {
      success = true;
    }
    {
      absl::MutexLock lk(&mtx);
      cv.SignalAll();
    }
  };
  client_instance_->endTransaction(target, metadata, request, absl::ToChronoMilliseconds(io_timeout_), cb);
  {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }
  return success;
}

void DefaultMQProducerImpl::isolatedEndpoints(absl::flat_hash_set<std::string>& endpoints) {
  absl::MutexLock lk(&isolated_endpoints_mtx_);
  endpoints.insert(isolated_endpoints_.begin(), isolated_endpoints_.end());
}

bool DefaultMQProducerImpl::isEndpointIsolated(const std::string& target) {
  absl::MutexLock lk(&isolated_endpoints_mtx_);
  return isolated_endpoints_.contains(target);
}

void DefaultMQProducerImpl::isolateEndpoint(const std::string& target) {
  absl::MutexLock lk(&isolated_endpoints_mtx_);
  isolated_endpoints_.insert(target);
}

std::unique_ptr<TransactionImpl> DefaultMQProducerImpl::prepare(const MQMessage& message) {
  try {
    SendResult send_result = send(message);
    return std::unique_ptr<TransactionImpl>(new TransactionImpl(send_result.getMsgId(), send_result.getTransactionId(),
                                                                send_result.getMessageQueue().serviceAddress(),
                                                                DefaultMQProducerImpl::shared_from_this()));
  } catch (const MQClientException& e) {
    SPDLOG_ERROR("Failed to send transaction message. Cause: {}", e.what());
    return nullptr;
  }
}

bool DefaultMQProducerImpl::commit(const std::string& message_id, const std::string& transaction_id,
                                   const std::string& target) {
  return endTransaction0(target, message_id, transaction_id, TransactionState::COMMIT);
}

bool DefaultMQProducerImpl::rollback(const std::string& message_id, const std::string& transaction_id,
                                     const std::string& target) {
  return endTransaction0(target, message_id, transaction_id, TransactionState::ROLLBACK);
}

void DefaultMQProducerImpl::asyncPublishInfo(const std::string& topic,
                                             const std::function<void(const TopicPublishInfoPtr&)>& cb) {
  TopicPublishInfoPtr ptr;
  {
    absl::MutexLock lock(&topic_publish_info_mtx_);
    if (topic_publish_info_table_.contains(topic)) {
      ptr = topic_publish_info_table_.at(topic);
    }
  }
  if (ptr) {
    cb(ptr);
  } else {
    auto callback = [this, topic, cb](const TopicRouteDataPtr& route) {
      if (!route) {
        cb(nullptr);
        return;
      }

      auto publish_info = std::make_shared<TopicPublishInfo>(topic, route);
      {
        absl::MutexLock lk(&topic_publish_info_mtx_);
        topic_publish_info_table_.insert_or_assign(topic, publish_info);
      }
      cb(publish_info);
    };
    getRouteFor(topic, callback);
  }
}

TopicPublishInfoPtr DefaultMQProducerImpl::getPublishInfo(const std::string& topic) {
  bool complete = false;
  absl::Mutex mtx;
  absl::CondVar cv;
  TopicPublishInfoPtr topic_publish_info;
  auto cb = [&](const TopicPublishInfoPtr& ptr) {
    absl::MutexLock lk(&mtx);
    topic_publish_info = ptr;
    complete = true;
    cv.SignalAll();
  };
  asyncPublishInfo(topic, cb);

  // Wait till acquiring topic publish info completes
  while (!complete) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }
  return topic_publish_info;
}

bool DefaultMQProducerImpl::executeMessageQueueSelector(const MQMessage& message, MessageQueueSelector* selector,
                                                        void* arg, std::vector<MQMessageQueue>& result) {
  TopicPublishInfoPtr publish_info;
  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;
  auto callback = [&](const TopicPublishInfoPtr& ptr) {
    absl::MutexLock lk(&mtx);
    publish_info = ptr;
    completed = true;
    cv.SignalAll();
  };
  asyncPublishInfo(message.getTopic(), callback);

  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }

  if (!publish_info) {
    THROW_MQ_EXCEPTION(MQClientException, "Failed to acquire topic route data", NO_TOPIC_ROUTE_INFO);
  }

  MQMessageQueue queue = selector->select(publish_info->getMessageQueueList(), message, arg);
  result.emplace_back(std::move(queue));
  return true;
}

void DefaultMQProducerImpl::takeMessageQueuesRoundRobin(const TopicPublishInfoPtr& publish_info,
                                                        std::vector<MQMessageQueue>& message_queues, int number) {
  assert(publish_info);
  absl::flat_hash_set<std::string> isolated;
  isolatedEndpoints(isolated);
  publish_info->takeMessageQueues(isolated, message_queues, number);
}

std::vector<MQMessageQueue> DefaultMQProducerImpl::getTopicMessageQueueInfo(const std::string& topic) {
  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;
  TopicPublishInfoPtr ptr;
  auto await_callback = [&](const TopicPublishInfoPtr& publish_info) {
    absl::MutexLock lk(&mtx);
    ptr = publish_info;
    completed = true;
    cv.SignalAll();
  };
  asyncPublishInfo(topic, await_callback);

  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }

  if (!ptr) {
    THROW_MQ_EXCEPTION(MQClientException, "Failed to acquire topic publish_info", NO_TOPIC_ROUTE_INFO);
  }

  return ptr->getMessageQueueList();
}

void DefaultMQProducerImpl::prepareHeartbeatData(HeartbeatRequest& request) {
  rmq::HeartbeatEntry entry;
  entry.mutable_producer_group()->mutable_group()->set_arn(arn_);
  entry.mutable_producer_group()->mutable_group()->set_name(group_name_);
  request.mutable_heartbeats()->Add(std::move(entry));
}

void DefaultMQProducerImpl::resolveOrphanedTransactionalMessage(const std::string& transaction_id,
                                                                const MQMessageExt& message) {
  if (transaction_state_checker_) {
    TransactionState state = transaction_state_checker_->checkLocalTransactionState(message);
    const std::string& target_host = MessageAccessor::targetEndpoint(message);
    endTransaction0(target_host, message.getMsgId(), transaction_id, state);
  } else {
    SPDLOG_WARN("LocalTransactionStateChecker is unexpectedly nullptr");
  }
}

ROCKETMQ_NAMESPACE_END