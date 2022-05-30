/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "ProducerImpl.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <limits>
#include <memory>
#include <system_error>
#include <utility>

#include "Client.h"
#include "MessageGroupQueueSelector.h"
#include "MetadataConstants.h"
#include "MixAll.h"
#include "Protocol.h"
#include "PublishInfoCallback.h"
#include "RpcClient.h"
#include "SendContext.h"
#include "SendMessageContext.h"
#include "Signature.h"
#include "Tag.h"
#include "TracingUtility.h"
#include "TransactionImpl.h"
#include "UniqueIdGenerator.h"
#include "UtilAll.h"
#include "absl/strings/str_join.h"
#include "opencensus/trace/propagation/trace_context.h"
#include "opencensus/trace/span.h"
#include "rocketmq/ErrorCode.h"
#include "rocketmq/Message.h"
#include "rocketmq/SendReceipt.h"
#include "rocketmq/Tracing.h"
#include "rocketmq/Transaction.h"
#include "rocketmq/TransactionChecker.h"

ROCKETMQ_NAMESPACE_BEGIN

ProducerImpl::ProducerImpl() : ClientImpl(""), compress_body_threshold_(MixAll::DEFAULT_COMPRESS_BODY_THRESHOLD_) {
  // TODO: initialize client_config_ and fault_strategy_
}

ProducerImpl::~ProducerImpl() {
  SPDLOG_INFO("Producer instance is destructed");
  shutdown();
}

void ProducerImpl::start() {
  ClientImpl::start();

  State expecting = State::STARTING;
  if (!state_.compare_exchange_strong(expecting, State::STARTED)) {
    SPDLOG_ERROR("Start with unexpected state. Expecting: {}, Actual: {}", State::STARTING,
                 state_.load(std::memory_order_relaxed));
    return;
  }

  client_manager_->addClientObserver(shared_from_this());
}

void ProducerImpl::shutdown() {
  State expected = State::STARTED;
  if (!state_.compare_exchange_strong(expected, State::STOPPING)) {
    SPDLOG_ERROR("Shutdown with unexpected state. Expecting: {}, Actual: {}", State::STOPPING,
                 state_.load(std::memory_order_relaxed));
    return;
  }

  notifyClientTermination();

  ClientImpl::shutdown();
  assert(State::STOPPED == state_.load());
  SPDLOG_INFO("Producer instance stopped");
}

void ProducerImpl::notifyClientTermination() {
}

bool ProducerImpl::isRunning() const {
  return State::STARTED == state_.load(std::memory_order_relaxed);
}

void ProducerImpl::ensureRunning(std::error_code& ec) const noexcept {
  if (!isRunning()) {
    ec = ErrorCode::IllegalState;
  }
}

bool ProducerImpl::validate(const Message& message) {
  return MixAll::validate(message);
}

void ProducerImpl::wrapSendMessageRequest(const Message& message, SendMessageRequest& request,
                                          const rmq::MessageQueue& message_queue) {
  auto msg = new rmq::Message();

  msg->mutable_topic()->set_resource_namespace(resourceNamespace());
  msg->mutable_topic()->set_name(message.topic());

  auto system_properties = msg->mutable_system_properties();

  // Handle Tag
  if (message.tag().has_value()) {
    system_properties->set_tag(message.tag().value());
  }

  // Handle Key
  const auto& keys = message.keys();
  if (!keys.empty()) {
    system_properties->mutable_keys()->Add(keys.begin(), keys.end());
  }

  // TraceContext
  if (message.traceContext().has_value()) {
    const auto& trace_context = message.traceContext().value();
    if (!trace_context.empty()) {
      system_properties->set_trace_context(trace_context);
    }
  }

  // Delivery Timestamp
  if (message.deliveryTimestamp().has_value()) {
    auto delivery_timestamp = message.deliveryTimestamp().value();
    if (delivery_timestamp.time_since_epoch().count()) {
      auto duration = delivery_timestamp.time_since_epoch();
      system_properties->set_delivery_attempt(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }
  }

  // Born-time
  auto duration = absl::Now() - absl::UnixEpoch();
  int64_t seconds = absl::ToInt64Seconds(duration);
  system_properties->mutable_born_timestamp()->set_seconds(seconds);
  system_properties->mutable_born_timestamp()->set_nanos(absl::ToInt64Nanoseconds(duration - absl::Seconds(seconds)));

  system_properties->set_born_host(UtilAll::hostname());

  if (message.deliveryTimestamp().has_value()) {
    system_properties->set_message_type(rmq::MessageType::DELAY);
  } else if (message.group().has_value()) {
    system_properties->set_message_type(rmq::MessageType::FIFO);
  } else {
    system_properties->set_message_type(rmq::MessageType::NORMAL);
  }

  if (message.body().size() >= compress_body_threshold_) {
    std::string compressed_body;
    UtilAll::compress(message.body(), compressed_body);
    msg->set_body(compressed_body);
    system_properties->set_body_encoding(rmq::Encoding::GZIP);
  } else {
    msg->set_body(message.body());
    system_properties->set_body_encoding(rmq::Encoding::IDENTITY);
  }

  if (message.group().has_value()) {
    system_properties->set_message_group(message.group().value());
  }

  system_properties->set_message_id(message.id());
  system_properties->set_queue_id(message_queue.id());

  // Forward user-defined-properties
  for (auto& item : message.properties()) {
    msg->mutable_user_properties()->insert({item.first, item.second});
  }

  // Add into request
  request.mutable_messages()->AddAllocated(msg);
  SPDLOG_TRACE("SendMessageRequest: {}", request.DebugString());
}

SendReceipt ProducerImpl::send(MessageConstPtr message, std::error_code& ec) noexcept {
  ensureRunning(ec);
  if (ec) {
    return {};
  }

  auto topic_publish_info = getPublishInfo(message->topic());
  if (!topic_publish_info) {
    ec = ErrorCode::NotFound;
    return {};
  }

  std::vector<rmq::MessageQueue> message_queue_list;
  if (!topic_publish_info->selectMessageQueues(absl::make_optional<std::string>(), message_queue_list)) {
    ec = ErrorCode::NotFound;
    return {};
  }

  auto mtx = std::make_shared<absl::Mutex>();
  auto cv = std::make_shared<absl::CondVar>();
  bool          completed = false;
  SendReceipt   send_receipt;

  // Define callback procedureq
  auto callback = [&, mtx, cv](const std::error_code& code, const SendReceipt& receipt) {
    ec = code;
    send_receipt = receipt;
    {
      absl::MutexLock lk(mtx.get());
      completed = true;
    }
    cv->Signal();
  };

  send0(std::move(message), callback, message_queue_list);

  {
    absl::MutexLock lk(mtx.get());
    if (!completed) {
      cv->Wait(mtx.get());
    }
  }

  return send_receipt;
}

void ProducerImpl::send(MessageConstPtr message, SendCallback cb) {
  std::error_code ec;
  ensureRunning(ec);
  if (ec) {
    SendReceipt send_receipt;
    cb(ec, send_receipt);
  }

  std::string topic = message->topic();
  std::weak_ptr<ProducerImpl> client(shared_from_this());

  // Walk-around: Use capture by move when C++14 is possible
  const Message* msg = message.release();
  auto callback = [client, cb, msg](const std::error_code& ec, const TopicPublishInfoPtr& publish_info) {
    MessageConstPtr ptr(msg);
    // No route entries of the given topic is available
    if (ec) {
      SendReceipt send_receipt;
      cb(ec, send_receipt);
      return;
    }

    if (!publish_info) {
      std::error_code ec = ErrorCode::NotFound;
      SendReceipt     send_receipt;
      cb(ec, send_receipt);
      return;
    }

    auto publisher = client.lock();
    std::vector<rmq::MessageQueue> message_queue_list;
    if (!publish_info->selectMessageQueues(ptr->group(), message_queue_list)) {
      std::error_code ec = ErrorCode::NotFound;
      SendReceipt     send_receipt;
      cb(ec, send_receipt);
      return;
    }

    publisher->send0(std::move(ptr), cb, message_queue_list);
  };

  getPublishInfoAsync(topic, callback);
}

void ProducerImpl::setTransactionChecker(TransactionChecker checker) {
  transaction_checker_ = std::move(checker);
}

void ProducerImpl::sendImpl(std::shared_ptr<SendContext> context) {
  const std::string& target = urlOf(context->messageQueue());
  if (target.empty()) {
    SPDLOG_WARN("Failed to resolve broker address from MessageQueue");
    std::error_code ec = ErrorCode::BadGateway;
    context->onFailure(ec);
    return;
  }

  {
    // Trace Send RPC
    if (context->message_->traceContext().has_value()) {
      auto span_context =
          opencensus::trace::propagation::FromTraceParentHeader(context->message_->traceContext().value());
      auto span = opencensus::trace::Span::BlankSpan();
      std::string span_name = resourceNamespace() + "/" + context->message_->topic() + " " +
                              MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_SEND_OPERATION;
      if (span_context.IsValid()) {
        span = opencensus::trace::Span::StartSpanWithRemoteParent(span_name, span_context, {traceSampler()});
      } else {
        span = opencensus::trace::Span::StartSpan(span_name, nullptr, {traceSampler()});
      }
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_OPERATION,
                        MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_SEND_OPERATION);
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_OPERATION,
                        MixAll::SPAN_ATTRIBUTE_VALUE_MESSAGING_SEND_OPERATION);
      TracingUtility::addUniversalSpanAttributes(*context->message_, config(), span);
      // Note: attempt-time is 0-based
      span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_ATTEMPT, 1 + context->attempt_times_);
      if (context->message_->deliveryTimestamp().has_value()) {
        span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_DELIVERY_TIMESTAMP,
                          absl::FormatTime(absl::FromChrono(context->message_->deliveryTimestamp().value())));
      }
      auto ptr = const_cast<Message*>(context->message_.get());
      ptr->traceContext(opencensus::trace::propagation::ToTraceParentHeader(span.context()));
      context->span_ = span;
    }
  }

  SendMessageRequest request;
  wrapSendMessageRequest(*context->message_, request, context->messageQueue());
  Metadata metadata;
  Signature::sign(client_config_, metadata);

  auto callback = [context](const std::error_code& ec, const SendReceipt& send_receipt) {
    if (ec) {
      context->onFailure(ec);
      return;
    }
    context->onSuccess(send_receipt);
  };

  client_manager_->send(target, metadata, request, callback);
}

void ProducerImpl::send0(MessageConstPtr message, SendCallback callback, std::vector<rmq::MessageQueue> list) {
  SendReceipt send_receipt;
  std::error_code ec;

  if (!validate(*message)) {
    ec = ErrorCode::BadRequest;
    callback(ec, send_receipt);
    return;
  }

  if (list.empty()) {
    ec = ErrorCode::NotFound;
    callback(ec, send_receipt);
    return;
  }

  auto context = std::make_shared<SendContext>(shared_from_this(), std::move(message), callback, std::move(list));
  sendImpl(context);
  // const_cast<Message&>(message).traceContext(
  //     opencensus::trace::propagation::ToTraceParentHeader(context->span().context()));
}

bool ProducerImpl::endTransaction0(const Transaction& transaction, TransactionState resolution) {
  EndTransactionRequest request;
  const std::string& topic = transaction.topic();
  request.mutable_topic()->set_name(topic);
  request.mutable_topic()->set_resource_namespace(resourceNamespace());
  request.set_message_id(transaction.messageId());
  request.set_transaction_id(transaction.messageId());

  std::string action;
  switch (resolution) {
    case TransactionState::COMMIT:
      request.set_resolution(rmq::COMMIT);
      action = "commit";
      break;
    case TransactionState::ROLLBACK:
      request.set_resolution(rmq::ROLLBACK);
      action = "rollback";
      break;
  }
  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(config(), metadata);
  bool completed = false;
  bool success = false;
  auto span = opencensus::trace::Span::BlankSpan();
  if (!transaction.traceContext().empty()) {
    // Trace transactional message
    opencensus::trace::SpanContext span_context =
        opencensus::trace::propagation::FromTraceParentHeader(transaction.traceContext());
    std::string trace_operation_name = TransactionState::COMMIT == resolution
                                           ? MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_COMMIT_OPERATION
                                           : MixAll::SPAN_ATTRIBUTE_VALUE_ROCKETMQ_ROLLBACK_OPERATION;
    std::string span_name = resourceNamespace() + "/" + transaction.topic() + " " + trace_operation_name;
    if (span_context.IsValid()) {
      span = opencensus::trace::Span::StartSpanWithRemoteParent(span_name, span_context, {traceSampler()});
    } else {
      span = opencensus::trace::Span::StartSpan(span_name, nullptr, {traceSampler()});
    }
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_MESSAGING_OPERATION, trace_operation_name);
    span.AddAttribute(MixAll::SPAN_ATTRIBUTE_KEY_ROCKETMQ_OPERATION, trace_operation_name);
    // TracingUtility::addUniversalSpanAttributes(message, config(), span);
  }

  auto mtx = std::make_shared<absl::Mutex>();
  auto cv = std::make_shared<absl::CondVar>();
  const auto& endpoint = transaction.endpoint();
  std::weak_ptr<ProducerImpl> publisher(shared_from_this());

  auto cb = [&, span, endpoint, mtx, cv, topic](const std::error_code& ec, const EndTransactionResponse& response) {
    if (ec) {
      {
        span.SetStatus(opencensus::trace::StatusCode::ABORTED);
        span.AddAnnotation(ec.message());
        span.End();
      }
      SPDLOG_WARN("Failed to send {} transaction request to {}. Cause: ", action, endpoint, ec.message());
      success = false;
    } else {
      {
        span.SetStatus(opencensus::trace::StatusCode::OK);
        span.End();
      }
      success = true;
    }

    {
      absl::MutexLock lk(mtx.get());
      completed = true;
      cv->SignalAll();
    }
  };

  client_manager_->endTransaction(transaction.endpoint(), metadata, request,
                                  absl::ToChronoMilliseconds(requestTimeout()), cb);
  {
    absl::MutexLock lk(mtx.get());
    cv->Wait(mtx.get());
  }
  return success;
}

void ProducerImpl::isolatedEndpoints(absl::flat_hash_set<std::string>& endpoints) {
  absl::MutexLock lk(&isolated_endpoints_mtx_);
  endpoints.insert(isolated_endpoints_.begin(), isolated_endpoints_.end());
}

bool ProducerImpl::isEndpointIsolated(const std::string& target) {
  absl::MutexLock lk(&isolated_endpoints_mtx_);
  return isolated_endpoints_.contains(target);
}

void ProducerImpl::isolateEndpoint(const std::string& target) {
  absl::MutexLock lk(&isolated_endpoints_mtx_);
  isolated_endpoints_.insert(target);
}

std::unique_ptr<TransactionImpl> ProducerImpl::prepare(MessageConstPtr message, std::error_code& ec) {
  std::weak_ptr<ProducerImpl> producer(shared_from_this());
  auto transaction = absl::make_unique<TransactionImpl>(message->topic(), message->id(),
                                                        message->traceContext().value_or(""), producer);
  SendReceipt send_receipt = send(std::move(message), ec);
  if (ec) {
    return nullptr;
  }

  transaction->transactionId(send_receipt.transaction_id);

  // TODO: endpoint id
  // transaction->endpoint(xxx);
  return transaction;
}

bool ProducerImpl::commit(const Transaction& transaction) {
  return endTransaction0(transaction, TransactionState::COMMIT);
}

bool ProducerImpl::rollback(const Transaction& transaction) {
  return endTransaction0(transaction, TransactionState::ROLLBACK);
}

void ProducerImpl::getPublishInfoAsync(const std::string& topic, const PublishInfoCallback& cb) {
  TopicPublishInfoPtr ptr;
  {
    absl::MutexLock lock(&topic_publish_info_mtx_);
    if (topic_publish_info_table_.contains(topic)) {
      ptr = topic_publish_info_table_.at(topic);
    }
  }

  std::error_code ec;
  if (ptr) {
    // Serve with the cached one
    cb(ec, ptr);
    return;
  }

  std::weak_ptr<ProducerImpl> producer(shared_from_this());
  auto callback = [topic, cb, producer](const std::error_code& ec, const TopicRouteDataPtr& route) {
    if (ec) {
      cb(ec, nullptr);
      return;
    }
    auto publish_info = std::make_shared<TopicPublishInfo>(producer, topic, route);
    auto impl         = producer.lock();
    if (impl) {
      impl->cachePublishInfo(topic, publish_info);
    }
    cb(ec, publish_info);
  };

  getRouteFor(topic, callback);
}

void ProducerImpl::cachePublishInfo(const std::string& topic, TopicPublishInfoPtr info) {
  absl::MutexLock lk(&topic_publish_info_mtx_);
  topic_publish_info_table_.insert_or_assign(topic, info);
}

TopicPublishInfoPtr ProducerImpl::getPublishInfo(const std::string& topic) {
  bool complete = false;
  auto mtx = std::make_shared<absl::Mutex>();
  auto cv = std::make_shared<absl::CondVar>();
  TopicPublishInfoPtr topic_publish_info;
  std::error_code error_code;
  auto cb = [&, mtx, cv](const std::error_code& ec, const TopicPublishInfoPtr& ptr) {
    absl::MutexLock lk(mtx.get());
    topic_publish_info = ptr;
    error_code = ec;
    complete = true;
    cv->SignalAll();
  };
  getPublishInfoAsync(topic, cb);

  // Wait till acquiring topic publish info completes
  while (!complete) {
    absl::MutexLock lk(mtx.get());
    cv->Wait(mtx.get());
  }

  // TODO: propogate error_code to caller
  return topic_publish_info;
}

void ProducerImpl::prepareHeartbeatData(HeartbeatRequest& request) {
  request.set_client_type(rmq::ClientType::PRODUCER);
}

void ProducerImpl::onOrphanedTransactionalMessage(MessageConstSharedPtr message) {
  if (transaction_checker_) {
    std::weak_ptr<ProducerImpl> producer(shared_from_this());

    auto transaction = absl::make_unique<TransactionImpl>(message->topic(), message->id(),
                                                          message->traceContext().value_or(""), producer);
    transaction->endpoint(message->extension().target_endpoint);
    transaction->transactionId(message->extension().transaction_id);
    TransactionState state = transaction_checker_(*message);
    endTransaction0(*transaction, state);
  } else {
    SPDLOG_WARN("LocalTransactionStateChecker is unexpectedly nullptr");
  }
}

void ProducerImpl::topicsOfInterest(std::vector<std::string> topics) {
  absl::MutexLock lk(&topics_mtx_);
  topics_.swap(topics);
}

void ProducerImpl::buildClientSettings(rmq::Settings& settings) {
  settings.set_client_type(rmq::ClientType::PRODUCER);

  auto topics = settings.mutable_publishing()->mutable_topics();
  {
    absl::MutexLock lk(&topics_mtx_);
    for (const auto& item : topics_) {
      auto topic = new rmq::Resource;
      topic->set_resource_namespace(resourceNamespace());
      topic->set_name(item);
      topics->AddAllocated(topic);
    }
  }
}

ROCKETMQ_NAMESPACE_END