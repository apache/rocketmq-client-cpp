#include "ProducerImpl.h"

#include <atomic>
#include <limits>
#include <system_error>
#include <utility>

#include "absl/strings/str_join.h"
#include "opencensus/trace/propagation/trace_context.h"
#include "opencensus/trace/span.h"

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
#include "rocketmq/ErrorCode.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQMessageQueue.h"
#include "rocketmq/Transaction.h"

ROCKETMQ_NAMESPACE_BEGIN

ProducerImpl::ProducerImpl(absl::string_view group_name)
    : ClientImpl(group_name), compress_body_threshold_(MixAll::DEFAULT_COMPRESS_BODY_THRESHOLD_) {
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

void ProducerImpl::ensureRunning(std::error_code& ec) const noexcept {
  if (!isRunning()) {
    ec = ErrorCode::IllegalState;
  }
}

bool ProducerImpl::validate(const MQMessage& message) { return MixAll::validate(message); }

std::string ProducerImpl::wrapSendMessageRequest(const MQMessage& message, SendMessageRequest& request,
                                                 const MQMessageQueue& message_queue) {
  request.mutable_message()->mutable_topic()->set_resource_namespace(resource_namespace_);
  request.mutable_message()->mutable_topic()->set_name(message.getTopic());

  auto system_attribute = request.mutable_message()->mutable_system_attribute();

  // Born-time
  auto duration = absl::Now() - absl::UnixEpoch();
  int64_t seconds = absl::ToInt64Seconds(duration);
  system_attribute->mutable_born_timestamp()->set_seconds(seconds);
  system_attribute->mutable_born_timestamp()->set_nanos(absl::ToInt64Nanoseconds(duration - absl::Seconds(seconds)));

  system_attribute->set_born_host(UtilAll::hostname());

  system_attribute->mutable_producer_group()->set_resource_namespace(resource_namespace_);
  system_attribute->mutable_producer_group()->set_name(group_name_);

  switch (message.messageType()) {
  case MessageType::NORMAL:
    system_attribute->set_message_type(rmq::MessageType::NORMAL);
    break;
  case MessageType::DELAY:
    system_attribute->set_message_type(rmq::MessageType::DELAY);
    break;
  case MessageType::TRANSACTION:
    system_attribute->set_message_type(rmq::MessageType::TRANSACTION);
    break;
  case MessageType::FIFO:
    system_attribute->set_message_type(rmq::MessageType::FIFO);
    break;
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
  SPDLOG_TRACE("SendMessageRequest: {}", request.DebugString());
  return message_id;
}

SendResult ProducerImpl::send(const MQMessage& message, std::error_code& ec) noexcept {
  ensureRunning(ec);
  if (ec) {
    return {};
  }

  auto topic_publish_info = getPublishInfo(message.getTopic());
  if (!topic_publish_info) {
    ec = ErrorCode::NotFound;
    return {};
  }

  std::vector<MQMessageQueue> message_queue_list;

  if (!message.messageGroup().empty() || message.messageQueue()) {
    auto&& list = listMessageQueue(message.getTopic(), ec);
    if (ec) {
      return {};
    }

    if (list.empty()) {
      ec = ErrorCode::ServiceUnavailable;
      return {};
    }

    if (!message.messageGroup().empty()) {
      std::size_t hash_code = std::hash<std::string>{}(message.messageGroup());
      hash_code = hash_code & std::numeric_limits<std::size_t>::max();
      std::size_t r = hash_code % list.size();
      message_queue_list.push_back(list[r]);
    } else {
      for (const auto& entry : list) {
        if (entry == message.messageQueue()) {
          message_queue_list.push_back(entry);
          break;
        }
      }
    }
  } else {
    takeMessageQueuesRoundRobin(topic_publish_info, message_queue_list, max_attempt_times_);
  }

  if (message_queue_list.empty()) {
    ec = ErrorCode::ServiceUnavailable;
    return {};
  }

  AwaitSendCallback callback;
  send0(message, &callback, message_queue_list, max_attempt_times_);
  callback.await();

  if (callback) {
    return callback.sendResult();
  }

  ec = callback.errorCode();
  return {};
}

void ProducerImpl::send(const MQMessage& message, SendCallback* cb) {
  std::error_code ec;
  ensureRunning(ec);
  if (ec) {
    cb->onFailure(ec);
  }

  auto callback = [this, message, cb](const std::error_code& ec, const TopicPublishInfoPtr& publish_info) {
    if (ec) {
      cb->onFailure(ec);
      return;
    }

    std::vector<MQMessageQueue> message_queue_list;
    if (!message.messageGroup().empty() || message.messageQueue()) {
      auto&& list = publish_info->getMessageQueueList();
      if (!message.messageGroup().empty()) {
        std::size_t hash_code = std::hash<std::string>{}(message.messageGroup());
        hash_code = hash_code & std::numeric_limits<std::size_t>::max();
        std::size_t r = hash_code % list.size();
        message_queue_list.push_back(list[r]);
      } else {
        for (const auto& entry : list) {
          if (entry == message.messageQueue()) {
            message_queue_list.push_back(entry);
            break;
          }
        }
      }
    } else {
      takeMessageQueuesRoundRobin(publish_info, message_queue_list, max_attempt_times_);
    }

    if (message_queue_list.empty()) {
      cb->onFailure(ErrorCode::ServiceUnavailable);
      return;
    }

    send0(message, cb, message_queue_list, max_attempt_times_);
  };

  asyncPublishInfo(message.getTopic(), callback);
}

void ProducerImpl::sendOneway(const MQMessage& message, std::error_code& ec) { send(message, ec); }

void ProducerImpl::setLocalTransactionStateChecker(LocalTransactionStateCheckerPtr checker) {
  transaction_state_checker_ = std::move(checker);
}

void ProducerImpl::sendImpl(RetrySendCallback* callback) {
  const std::string& target = callback->messageQueue().serviceAddress();
  if (target.empty()) {
    SPDLOG_WARN("Failed to resolve broker address from MessageQueue");
    std::error_code ec = ErrorCode::BadGateway;
    callback->onFailure(ec);
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
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_ARN, resourceNamespace());
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

  if (!validate(message)) {
    std::error_code ec = ErrorCode::BadRequest;
    callback->onFailure(ec);
    return;
  }

  if (list.empty()) {
    std::error_code ec = ErrorCode::NotFound;
    callback->onFailure(ec);
    return;
  }

  if (max_attempt_times <= 0) {
    std::error_code ec = ErrorCode::BadConfiguration;
    callback->onFailure(ec);
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
  span.AddAttribute(MixAll::SPAN_ATTRIBUTE_ARN, resourceNamespace());
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
  auto cb = [&, span](const std::error_code& ec, const EndTransactionResponse& response) {
    completed = true;
    if (ec) {
      {
        span.SetStatus(opencensus::trace::StatusCode::ABORTED);
        span.AddAnnotation(ec.message());
        span.End();
      }
      SPDLOG_WARN("Failed to send {} transaction request to {}. Cause: ", action, target, ec.message());
      success = false;
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

MQMessageQueue ProducerImpl::withServiceAddress(const MQMessageQueue& message_queue, std::error_code& ec) {
  if (!message_queue.serviceAddress().empty()) {
    return message_queue;
  }

  if (message_queue.getTopic().empty() || message_queue.getBrokerName().empty() || message_queue.getQueueId() < 0) {
    ec = ErrorCode::BadRequest;
    return {};
  }

  std::vector<MQMessageQueue> list = listMessageQueue(message_queue.getTopic(), ec);
  if (ec) {
    return {};
  }

  for (const auto& item : list) {
    if (item == message_queue) {
      return item;
    }
  }

  if (list.empty()) {
    ec = ErrorCode::NotFound;
    return {};
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

std::unique_ptr<TransactionImpl> ProducerImpl::prepare(MQMessage& message, std::error_code& ec) {
  message.messageType(MessageType::TRANSACTION);
  SendResult send_result = send(message, ec);
  if (ec) {
    return nullptr;
  }

  return std::unique_ptr<TransactionImpl>(new TransactionImpl(
      send_result.getMsgId(), send_result.getTransactionId(), send_result.getMessageQueue().serviceAddress(),
      send_result.traceContext(), ProducerImpl::shared_from_this()));
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
                                    const std::function<void(const std::error_code&, const TopicPublishInfoPtr&)>& cb) {
  TopicPublishInfoPtr ptr;
  {
    absl::MutexLock lock(&topic_publish_info_mtx_);
    if (topic_publish_info_table_.contains(topic)) {
      ptr = topic_publish_info_table_.at(topic);
    }
  }
  std::error_code ec;
  if (ptr) {
    cb(ec, ptr);
  } else {
    auto callback = [this, topic, cb](const std::error_code& ec, const TopicRouteDataPtr& route) {
      if (ec) {
        cb(ec, nullptr);
        return;
      }

      auto publish_info = std::make_shared<TopicPublishInfo>(topic, route);
      {
        absl::MutexLock lk(&topic_publish_info_mtx_);
        topic_publish_info_table_.insert_or_assign(topic, publish_info);
      }
      cb(ec, publish_info);
    };

    getRouteFor(topic, callback);
  }
}

TopicPublishInfoPtr ProducerImpl::getPublishInfo(const std::string& topic) {
  bool complete = false;
  absl::Mutex mtx;
  absl::CondVar cv;
  TopicPublishInfoPtr topic_publish_info;
  std::error_code error_code;
  auto cb = [&](const std::error_code& ec, const TopicPublishInfoPtr& ptr) {
    absl::MutexLock lk(&mtx);
    topic_publish_info = ptr;
    error_code = ec;
    complete = true;
    cv.SignalAll();
  };
  asyncPublishInfo(topic, cb);

  // Wait till acquiring topic publish info completes
  while (!complete) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }

  // TODO: propogate error_code to caller
  return topic_publish_info;
}

void ProducerImpl::takeMessageQueuesRoundRobin(const TopicPublishInfoPtr& publish_info,
                                               std::vector<MQMessageQueue>& message_queues, int number) {
  assert(publish_info);
  absl::flat_hash_set<std::string> isolated;
  isolatedEndpoints(isolated);
  publish_info->takeMessageQueues(isolated, message_queues, number);
}

std::vector<MQMessageQueue> ProducerImpl::listMessageQueue(const std::string& topic, std::error_code& ec) {
  absl::Mutex mtx;
  absl::CondVar cv;
  bool completed = false;
  TopicPublishInfoPtr ptr;
  auto await_callback = [&](const std::error_code& error_code, const TopicPublishInfoPtr& publish_info) {
    absl::MutexLock lk(&mtx);
    ptr = publish_info;
    ec = error_code;
    completed = true;
    cv.SignalAll();
  };

  asyncPublishInfo(topic, await_callback);

  while (!completed) {
    absl::MutexLock lk(&mtx);
    cv.Wait(&mtx);
  }

  if (ec) {
    return {};
  }

  return ptr->getMessageQueueList();
}

void ProducerImpl::prepareHeartbeatData(HeartbeatRequest& request) {
  request.set_client_id(clientId());
  request.mutable_producer_data()->mutable_group()->set_resource_namespace(resource_namespace_);
  request.mutable_producer_data()->mutable_group()->set_name(group_name_);
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