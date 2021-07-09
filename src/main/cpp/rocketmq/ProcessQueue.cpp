#include "ProcessQueue.h"
#include "ClientInstance.h"
#include "DefaultMQPushConsumerImpl.h"
#include "Metadata.h"
#include "Protocol.h"
#include "Signature.h"
#include <arpa/inet.h>
#include <chrono>
#include <memory>
#include <utility>

using namespace std::chrono;

ROCKETMQ_NAMESPACE_BEGIN

ProcessQueue::ProcessQueue(MQMessageQueue message_queue, FilterExpression filter_expression,
                           ConsumeMessageType consume_type, int max_cache_size,
                           std::weak_ptr<DefaultMQPushConsumerImpl> call_back_owner,
                           std::shared_ptr<ClientInstance> client_instance)
    : message_queue_(std::move(message_queue)), filter_expression_(std::move(filter_expression)),
      consume_type_(consume_type), max_message_number_(MixAll::MAX_MESSAGE_NUMBER_PER_BATCH),
      invisible_time_(MixAll::millisecondsOf(MixAll::DEFAULT_INVISIBLE_TIME_)), message_cached_number_(0),
      max_cache_size_(max_cache_size), simple_name_(message_queue_.simpleName()),
      call_back_owner_(std::move(call_back_owner)), client_instance_(std::move(client_instance)) {
  SPDLOG_DEBUG("Created ProcessQueue={}", simpleName());
}

ProcessQueue::~ProcessQueue() {
  SPDLOG_INFO("ProcessQueue={} should have been re-balanced away, thus, is destructed", simpleName());
}

void ProcessQueue::callback(std::shared_ptr<ReceiveMessageCallback> callback) { callback_ = std::move(callback); }

bool ProcessQueue::expired() const {
  auto duration = std::chrono::steady_clock::now() - last_poll_timestamp_;
  auto throttle_duration = std::chrono::steady_clock::now() - last_throttle_timestamp_;
  if (duration > MixAll::PROCESS_QUEUE_EXPIRATION_THRESHOLD_ &&
      throttle_duration > MixAll::PROCESS_QUEUE_EXPIRATION_THRESHOLD_) {
    SPDLOG_WARN("ProcessQueue={} is expired. Duration from last poll is: {}ms; from last throttle is: {}ms",
                simpleName(), MixAll::millisecondsOf(duration), MixAll::millisecondsOf(throttle_duration));
    return true;
  }
  return false;
}

bool ProcessQueue::shouldThrottle() const {
  int current = message_cached_number_.load(std::memory_order_relaxed);
  bool need_throttle = current >= max_cache_size_;
  if (need_throttle) {
    SPDLOG_INFO("{}: Number of locally cached messages is {}, which exceeds threshold={}", simple_name_, current,
                max_cache_size_);
  }
  return need_throttle;
}

void ProcessQueue::receiveMessage() {
  switch (consume_type_) {
  case ConsumeMessageType::POP:
    popMessage();
    break;
  case ConsumeMessageType::PULL:
    pullMessage();
    break;
  }
}

void ProcessQueue::popMessage() {
  rmq::ReceiveMessageRequest request;

  absl::flat_hash_map<std::string, std::string> metadata;
  auto consumer_client = call_back_owner_.lock();
  if (!consumer_client) {
    return;
  }
  Signature::sign(consumer_client.get(), metadata);

  wrapPopMessageRequest(metadata, request);
  last_poll_timestamp_ = std::chrono::steady_clock::now();
  SPDLOG_DEBUG("Try to pop message from {}", message_queue_.simpleName());
  client_instance_->receiveMessage(message_queue_.serviceAddress(), metadata, request,
                                   absl::ToChronoMilliseconds(consumer_client->getIoTimeout()), callback_);
}

void ProcessQueue::pullMessage() {
  rmq::PullMessageRequest request;
  absl::flat_hash_map<std::string, std::string> metadata;
  wrapPullMessageRequest(metadata, request);
  client_instance_->pullMessage(message_queue_.serviceAddress(), metadata, request, callback_);
}

void ProcessQueue::cacheMessages(const std::vector<MQMessageExt>& messages) {
  absl::MutexLock lock(&cached_messages_mtx_);
  cached_messages_.reserve(cached_messages_.size() + messages.size());
  cached_messages_.insert(cached_messages_.end(), messages.begin(), messages.end());
  message_cached_number_.fetch_add(messages.size(), std::memory_order_relaxed);
  SPDLOG_DEBUG("{}: Number of locally cached messages is: {}", message_queue_.simpleName(),
               message_cached_number_.load(std::memory_order_relaxed));
}

bool ProcessQueue::consume(int batch_size, std::vector<MQMessageExt>& messages) {
  absl::MutexLock lock(&cached_messages_mtx_);
  if (cached_messages_.empty()) {
    return false;
  }

  for (auto it = cached_messages_.begin(); it != cached_messages_.end();) {
    if (--batch_size < 0) {
      break;
    }

    messages.push_back(*it);
    it = cached_messages_.erase(it);
  }
  return !cached_messages_.empty();
}

void ProcessQueue::wrapPopMessageRequest(absl::flat_hash_map<std::string, std::string>& metadata,
                                         rmq::ReceiveMessageRequest& request) {
  std::shared_ptr<DefaultMQPushConsumerImpl> consumer = call_back_owner_.lock();
  assert(consumer);
  request.set_client_id(consumer->clientId());
  request.mutable_group()->set_name(consumer->getGroupName());
  request.mutable_group()->set_arn(consumer->arn());
  request.mutable_partition()->set_id(message_queue_.getQueueId());
  request.mutable_partition()->mutable_topic()->set_name(message_queue_.getTopic());
  request.mutable_partition()->mutable_topic()->set_arn(consumer->arn());

  auto search = consumer->getTopicFilterExpressionTable().find(message_queue_.getTopic());
  if (consumer->getTopicFilterExpressionTable().end() != search) {
    FilterExpression expression = search->second;
    switch (expression.type_) {
    case TAG:
      request.mutable_filter_expression()->set_type(rmq::FilterType::TAG);
      request.mutable_filter_expression()->set_expression(expression.content_);
      break;
    case SQL92:
      request.mutable_filter_expression()->set_type(rmq::FilterType::SQL);
      request.mutable_filter_expression()->set_expression(expression.content_);
      break;
    }
  } else {
    request.mutable_filter_expression()->set_type(rmq::FilterType::TAG);
    request.mutable_filter_expression()->set_expression("*");
  }

  // Batch size
  request.set_batch_size(max_message_number_);

  // Consume policy
  request.set_consume_policy(rmq::ConsumePolicy::RESUME);

  // Set invisible time
  request.mutable_invisible_duration()->set_seconds(
      std::chrono::duration_cast<std::chrono::seconds>(invisible_time_).count());
  auto fraction = invisible_time_ - std::chrono::duration_cast<std::chrono::seconds>(invisible_time_);
  int32_t nano_seconds = static_cast<int32_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(fraction).count());
  request.mutable_invisible_duration()->set_nanos(nano_seconds);
}

void ProcessQueue::wrapPullMessageRequest(absl::flat_hash_map<std::string, std::string>& metadata,
                                          rmq::PullMessageRequest& request) {
  std::shared_ptr<DefaultMQPushConsumerImpl> consumer = call_back_owner_.lock();
  assert(consumer);
  request.set_client_id(consumer->clientId());
  request.mutable_group()->set_name(consumer->getGroupName());
  request.mutable_group()->set_arn(consumer->arn());
  request.mutable_partition()->set_id(message_queue_.getQueueId());
  request.mutable_partition()->mutable_topic()->set_name(message_queue_.getTopic());
  request.mutable_partition()->mutable_topic()->set_arn(consumer->arn());
  request.set_offset(next_offset_);
  request.set_batch_size(consumer->receiveBatchSize());
  auto filter_expression_table = consumer->getTopicFilterExpressionTable();
  auto search = filter_expression_table.find(message_queue_.getTopic());
  if (filter_expression_table.end() != search) {
    FilterExpression expression = search->second;
    switch (expression.type_) {
    case TAG:
      request.mutable_filter_expression()->set_type(rmq::FilterType::TAG);
      request.mutable_filter_expression()->set_expression(expression.content_);
      break;
    case SQL92:
      request.mutable_filter_expression()->set_type(rmq::FilterType::SQL);
      request.mutable_filter_expression()->set_expression(expression.content_);
      break;
    }
  } else {
    request.mutable_filter_expression()->set_type(rmq::FilterType::TAG);
    request.mutable_filter_expression()->set_expression("*");
  }
}

std::weak_ptr<DefaultMQPushConsumerImpl> ProcessQueue::getCallbackOwner() { return call_back_owner_; }

std::shared_ptr<ClientInstance> ProcessQueue::getClientInstance() { return client_instance_; }

MQMessageQueue ProcessQueue::getMQMessageQueue() { return message_queue_; }

const FilterExpression& ProcessQueue::getFilterExpression() const { return filter_expression_; }

ROCKETMQ_NAMESPACE_END