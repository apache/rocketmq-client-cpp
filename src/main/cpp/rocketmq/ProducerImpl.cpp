#include "ProducerImpl.h"

#include "MessageAccessor.h"
#include "MessageGroupQueueSelector.h"
#include "MetadataConstants.h"
#include "MixAll.h"
#include "OtlpExporter.h"
#include "Protocol.h"
#include "RpcClient.h"
#include "SendCallbacks.h"
#include "SendMessageContext.h"
#include "Signature.h"
#include "TransactionImpl.h"
#include "UniqueIdGenerator.h"
#include "UtilAll.h"
#include "absl/strings/str_join.h"
#include "opencensus/trace/propagation/trace_context.h"
#include "opencensus/trace/span.h"
#include "rocketmq/ErrorCode.h"
#include "rocketmq/MQClientException.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQMessageQueue.h"
#include "rocketmq/Transaction.h"
#include <atomic>

ROCKETMQ_NAMESPACE_BEGIN

ProducerImpl::ProducerImpl(std::string group_name)
    : ClientImpl(std::move(group_name)), compress_body_threshold_(MixAll::DEFAULT_COMPRESS_BODY_THRESHOLD_) {
  // TODO: initialize client_config_ and fault_strategy_
}

ProducerImpl::~ProducerImpl() { SPDLOG_INFO("Producer instance is destructed"); }

void ProducerImpl::start() {
  ClientImpl::start();
  if (State::STARTED != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected producer state: {}", state_.load(std::memory_order_relaxed));
    return;
  }

  client_manager_->addClientObserver(shared_from_this());
}

void ProducerImpl::shutdown() {
  ClientImpl::shutdown();

  State expected = State::STOPPING;
  if (state_.compare_exchange_strong(expected, State::STOPPED)) {
    SPDLOG_INFO("DefaultMQProducerImpl stopped");
  }
}

bool ProducerImpl::isRunning() const { return State::STARTED == state_.load(std::memory_order_relaxed); }

void ProducerImpl::ensureRunning() const {
  if (!isRunning()) {
    THROW_MQ_EXCEPTION(MQClientException, "Invoke #start() first", ILLEGAL_STATE);
  }
}

void ProducerImpl::validate(const MQMessage& message) {}

std::string ProducerImpl::wrapSendMessageRequest(const MQMessage& message, SendMessageRequest& request,
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

SendResult ProducerImpl::send(const MQMessage& message) {
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

SendResult ProducerImpl::send(const MQMessage& message, const std::string& message_group) {
  MessageGroupQueueSelector selector(message_group);
  return send(message, &selector, nullptr);
}

SendResult ProducerImpl::send(const MQMessage& message, const MQMessageQueue& message_queue) {
  ensureRunning();
  std::vector<MQMessageQueue> message_queue_list{withServiceAddress(message_queue)};
  AwaitSendCallback callback;
  send0(message, &callback, message_queue_list, max_attempt_times_);
  callback.await();
  if (callback) {
    return callback.sendResult();
  }
  THROW_MQ_EXCEPTION(MQClientException, callback.errorMessage(), FAILED_TO_SEND_MESSAGE);
}

SendResult ProducerImpl::send(const MQMessage& message, MessageQueueSelector* selector, void* arg) {
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

SendResult ProducerImpl::send(const MQMessage& message, MessageQueueSelector* selector, void* arg, int max_attempts) {
  ensureRunning();
  std::vector<MQMessageQueue> message_queue_list;
  executeMessageQueueSelector(message, selector, arg, message_queue_list);
  if (message_queue_list.empty()) {
    THROW_MQ_EXCEPTION(MQClientException, "No topic route available", NO_TOPIC_ROUTE_INFO);
  }
  AwaitSendCallback callback;
  send0(message, &callback, message_queue_list, max_attempts);
  callback.await();
  if (callback) {
    return callback.sendResult();
  }
  THROW_MQ_EXCEPTION(MQClientException, callback.errorMessage(), FAILED_TO_SEND_MESSAGE);
}

void ProducerImpl::send(const MQMessage& message, SendCallback* cb) {
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

void ProducerImpl::send(const MQMessage& message, const MQMessageQueue& message_queue, SendCallback* callback) {
  ensureRunning();
  std::vector<MQMessageQueue> message_queue_list{withServiceAddress(message_queue)};
  send0(message, callback, message_queue_list, max_attempt_times_);
}

void ProducerImpl::send(const MQMessage& message, MessageQueueSelector* selector, void* arg, SendCallback* callback) {
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

void ProducerImpl::sendOneway(const MQMessage& message) {
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

void ProducerImpl::sendOneway(const MQMessage& message, const MQMessageQueue& message_queue) {
  ensureRunning();
  std::vector<MQMessageQueue> list{withServiceAddress(message_queue)};
  send0(message, onewaySendCallback(), list, 1);
}

void ProducerImpl::sendOneway(const MQMessage& message, MessageQueueSelector* selector, void* arg) {
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

void ProducerImpl::setLocalTransactionStateChecker(LocalTransactionStateCheckerPtr checker) {
  transaction_state_checker_ = std::move(checker);
}

void ProducerImpl::sendImpl(RetrySendCallback* callback) {
  const std::string& target = callback->messageQueue().serviceAddress();
  if (target.empty()) {
    SPDLOG_WARN("Failed to resolve broker address from MessageQueue");
    MQClientException e("Failed to resolve broker address", FAILED_TO_RESOLVE_BROKER_ADDRESS_FROM_TOPIC_ROUTE, __FILE__,
                        __LINE__);
    callback->onException(e);
    return;
  }

  SendMessageRequest request;
  wrapSendMessageRequest(callback->message(), request, callback->messageQueue());
  Metadata metadata;
  Signature::sign(this, metadata);

  {
    // Trace Send RPC
    auto& message = callback->message();
    auto span_context = opencensus::trace::propagation::FromTraceParentHeader(message.traceContext());

    auto span = opencensus::trace::Span::BlankSpan();
    if (span_context.IsValid()) {
      span = opencensus::trace::Span::StartSpanWithRemoteParent(MixAll::SPAN_NAME_SEND_MESSAGE, span_context,
                                                                {&Samplers::always()});
    } else {
      span = opencensus::trace::Span::StartSpan(MixAll::SPAN_NAME_SEND_MESSAGE, nullptr, {&Samplers::always()});
    }

    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_ACCESS_KEY, credentialsProvider()->getCredentials().accessKey());
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_ARN, arn());
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_TOPIC, message.getTopic());
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_MESSAGE_ID, message.getMsgId());
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_GROUP, getGroupName());
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_TAG, message.getTags());
    const auto& keys = callback->message().getKeys();
    if (!keys.empty()) {
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEYS,
                        absl::StrJoin(keys.begin(), keys.end(), MixAll::MESSAGE_KEY_SEPARATOR));
    }
    // Note: attempt-time is 0-based
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_ATTEMPT_TIME, 1 + callback->attemptTime());
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_HOST, UtilAll::hostname());

    if (message.deliveryTimestamp() != absl::ToChronoTime(absl::UnixEpoch())) {
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_DELIVERY_TIMESTAMP,
                        absl::FormatTime(absl::FromChrono(message.deliveryTimestamp())));
    }
    callback->message().traceContext(opencensus::trace::propagation::ToTraceParentHeader(span.context()));
    callback->span() = span;
  }
  client_manager_->send(target, metadata, request, callback);
}

void ProducerImpl::send0(const MQMessage& message, SendCallback* callback, std::vector<MQMessageQueue> list,
                         int max_attempt_times) {
  assert(callback);
  if (list.empty()) {
    MQClientException e("Topic route not found", NO_TOPIC_ROUTE_INFO, __FILE__, __LINE__);
    callback->onException(e);
    return;
  }

  if (max_attempt_times <= 0) {
    MQClientException e("Retry times illegal", ERR_INVALID_MAX_ATTEMPT_TIME, __FILE__, __LINE__);
    callback->onException(e);
    return;
  }
  MQMessageQueue message_queue = list[0];
  auto retry_callback =
      new RetrySendCallback(shared_from_this(), message, max_attempt_times, callback, std::move(list));
  sendImpl(retry_callback);
}

bool ProducerImpl::endTransaction0(const std::string& target, const std::string& message_id,
                                   const std::string& transaction_id, TransactionState resolution,
                                   std::string trace_context) {

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
  // Trace transactional message
  opencensus::trace::SpanContext span_context = opencensus::trace::propagation::FromTraceParentHeader(trace_context);
  auto span = opencensus::trace::Span::BlankSpan();
  if (span_context.IsValid()) {
    span = opencensus::trace::Span::StartSpanWithRemoteParent(MixAll::SPAN_NAME_END_TRANSACTION, span_context,
                                                              {&Samplers::always()});
  } else {
    span = opencensus::trace::Span::StartSpan(MixAll::SPAN_NAME_END_TRANSACTION, nullptr, {&Samplers::always()});
  }

  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_ACCESS_KEY, credentialsProvider()->getCredentials().accessKey());
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_ARN, arn());
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_GROUP, getGroupName());
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_MESSAGE_ID, message_id);
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_HOST, UtilAll::hostname());
  switch (resolution) {
  case TransactionState::COMMIT:
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_TRANSACTION_RESOLUTION, "commit");
    break;
  case TransactionState::ROLLBACK:
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_TRANSACTION_RESOLUTION, "rollback");
    break;
  }

  absl::Mutex mtx;
  absl::CondVar cv;
  auto cb = [&, span](bool rpc_ok, const EndTransactionResponse& response) {
    completed = true;
    if (!rpc_ok) {
      {
        span.SetStatus(opencensus::trace::StatusCode::ABORTED);
        span.AddAnnotation("gRPC tier failure");
        span.End();
      }

      SPDLOG_WARN("Failed to send {} transaction request to {}", action, target);
      success = false;
    } else if (response.common().status().code() != google::rpc::Code::OK) {
      {
        span.SetStatus(opencensus::trace::ABORTED);
        span.AddAnnotation(response.common().status().DebugString());
        span.End();
      }
      success = false;
      SPDLOG_WARN("Server[host={}] failed to {} transaction. Reason: {}", target, action,
                  response.common().DebugString());
    } else {
      {
        span.SetStatus(opencensus::trace::StatusCode::OK);
        span.End();
      }
      success = true;
    }

    {
      absl::MutexLock lk(&mtx);
      cv.SignalAll();
    }
  };

  client_manager_->endTransaction(target, metadata, request, absl::ToChronoMilliseconds(io_timeout_), cb);
  {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }
  return success;
}

void ProducerImpl::isolatedEndpoints(absl::flat_hash_set<std::string>& endpoints) {
  absl::MutexLock lk(&isolated_endpoints_mtx_);
  endpoints.insert(isolated_endpoints_.begin(), isolated_endpoints_.end());
}

MQMessageQueue ProducerImpl::withServiceAddress(const MQMessageQueue& message_queue) {
  if (!message_queue.serviceAddress().empty()) {
    return message_queue;
  }

  if (message_queue.getTopic().empty() || message_queue.getBrokerName().empty() || message_queue.getQueueId() < 0) {
    MQClientException e("Message queue is illegal", MESSAGE_QUEUE_ILLEGAL, __FILE__, __LINE__);
    throw e;
  }

  std::vector<MQMessageQueue> list = getTopicMessageQueueInfo(message_queue.getTopic());
  for (const auto& item : list) {
    if (item == message_queue) {
      return item;
    }
  }

  if (list.empty()) {
    MQClientException e("No topic route available", NO_TOPIC_ROUTE_INFO, __FILE__, __LINE__);
    throw e;
  } else {
    return *list.begin();
  }
}

bool ProducerImpl::isEndpointIsolated(const std::string& target) {
  absl::MutexLock lk(&isolated_endpoints_mtx_);
  return isolated_endpoints_.contains(target);
}

void ProducerImpl::isolateEndpoint(const std::string& target) {
  absl::MutexLock lk(&isolated_endpoints_mtx_);
  isolated_endpoints_.insert(target);
}

std::unique_ptr<TransactionImpl> ProducerImpl::prepare(const MQMessage& message) {
  try {
    SendResult send_result = send(message);
    return std::unique_ptr<TransactionImpl>(new TransactionImpl(
        send_result.getMsgId(), send_result.getTransactionId(), send_result.getMessageQueue().serviceAddress(),
        send_result.traceContext(), ProducerImpl::shared_from_this()));
  } catch (const MQClientException& e) {
    SPDLOG_ERROR("Failed to send transaction message. Cause: {}", e.what());
    return nullptr;
  }
}

bool ProducerImpl::commit(const std::string& message_id, const std::string& transaction_id,
                          const std::string& trace_context, const std::string& target) {
  return endTransaction0(target, message_id, transaction_id, TransactionState::COMMIT, trace_context);
}

bool ProducerImpl::rollback(const std::string& message_id, const std::string& transaction_id,
                            const std::string& trace_context, const std::string& target) {
  return endTransaction0(target, message_id, transaction_id, TransactionState::ROLLBACK, trace_context);
}

void ProducerImpl::asyncPublishInfo(const std::string& topic,
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

TopicPublishInfoPtr ProducerImpl::getPublishInfo(const std::string& topic) {
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

bool ProducerImpl::executeMessageQueueSelector(const MQMessage& message, MessageQueueSelector* selector, void* arg,
                                               std::vector<MQMessageQueue>& result) {
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

void ProducerImpl::takeMessageQueuesRoundRobin(const TopicPublishInfoPtr& publish_info,
                                               std::vector<MQMessageQueue>& message_queues, int number) {
  assert(publish_info);
  absl::flat_hash_set<std::string> isolated;
  isolatedEndpoints(isolated);
  publish_info->takeMessageQueues(isolated, message_queues, number);
}

std::vector<MQMessageQueue> ProducerImpl::getTopicMessageQueueInfo(const std::string& topic) {
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

void ProducerImpl::prepareHeartbeatData(HeartbeatRequest& request) {
  rmq::HeartbeatEntry entry;
  entry.mutable_producer_group()->mutable_group()->set_arn(arn_);
  entry.mutable_producer_group()->mutable_group()->set_name(group_name_);
  request.mutable_heartbeats()->Add(std::move(entry));
}

void ProducerImpl::resolveOrphanedTransactionalMessage(const std::string& transaction_id, const MQMessageExt& message) {
  if (transaction_state_checker_) {
    TransactionState state = transaction_state_checker_->checkLocalTransactionState(message);
    const std::string& target_host = MessageAccessor::targetEndpoint(message);
    endTransaction0(target_host, message.getMsgId(), transaction_id, state, message.traceContext());
  } else {
    SPDLOG_WARN("LocalTransactionStateChecker is unexpectedly nullptr");
  }
}

ROCKETMQ_NAMESPACE_END