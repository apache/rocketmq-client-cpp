#include "ProcessQueueImpl.h"
#include "ClientManagerImpl.h"
#include "MetadataConstants.h"
#include "Protocol.h"
#include "PushConsumer.h"
#include "Signature.h"
#include <atomic>
#include <chrono>
#include <memory>
#include <utility>

using namespace std::chrono;

ROCKETMQ_NAMESPACE_BEGIN

ProcessQueueImpl::ProcessQueueImpl(MQMessageQueue message_queue, FilterExpression filter_expression,
                                   ConsumeMessageType consume_type, std::weak_ptr<PushConsumer> consumer,
                                   std::shared_ptr<ClientManager> client_instance)
    : message_queue_(std::move(message_queue)), filter_expression_(std::move(filter_expression)),
      consume_type_(consume_type), invisible_time_(MixAll::millisecondsOf(MixAll::DEFAULT_INVISIBLE_TIME_)),
      simple_name_(message_queue_.simpleName()), consumer_(std::move(consumer)),
      client_manager_(std::move(client_instance)), cached_message_quantity_(0), cached_message_memory_(0) {
  SPDLOG_DEBUG("Created ProcessQueue={}", simpleName());
}

ProcessQueueImpl::~ProcessQueueImpl() {
  SPDLOG_INFO("ProcessQueue={} should have been re-balanced away, thus, is destructed", simpleName());
}

void ProcessQueueImpl::callback(std::shared_ptr<ReceiveMessageCallback> callback) {
  receive_callback_ = std::move(callback);
}

bool ProcessQueueImpl::expired() const {
  auto duration = std::chrono::steady_clock::now() - idle_since_;
  if (duration > MixAll::PROCESS_QUEUE_EXPIRATION_THRESHOLD_) {
    SPDLOG_WARN("ProcessQueue={} is expired. It remains idle for {}ms", simpleName(), MixAll::millisecondsOf(duration));
    return true;
  }
  return false;
}

bool ProcessQueueImpl::shouldThrottle() const {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return false;
  }

  std::size_t quantity = cached_message_quantity_.load(std::memory_order_relaxed);
  uint32_t quantity_threshold = consumer->maxCachedMessageQuantity();
  uint64_t memory_threshold = consumer->maxCachedMessageMemory();
  bool need_throttle = quantity >= quantity_threshold;
  if (need_throttle) {
    SPDLOG_INFO("{}: Number of locally cached messages is {}, which exceeds threshold={}", simple_name_, quantity,
                quantity_threshold);
    return true;
  }

  if (memory_threshold) {
    uint64_t bytes = cached_message_memory_.load(std::memory_order_relaxed);
    need_throttle = bytes >= memory_threshold;
    if (need_throttle) {
      SPDLOG_INFO("{}: Locally cached messages take {} bytes, which exceeds threshold={}", simple_name_, bytes,
                  memory_threshold);
      return true;
    }
  }
  return false;
}

void ProcessQueueImpl::receiveMessage() {
  switch (consume_type_) {
  case ConsumeMessageType::POP:
    popMessage();
    break;
  case ConsumeMessageType::PULL:
    pullMessage();
    break;
  }
}

void ProcessQueueImpl::popMessage() {
  rmq::ReceiveMessageRequest request;

  absl::flat_hash_map<std::string, std::string> metadata;
  auto consumer_client = consumer_.lock();
  if (!consumer_client) {
    return;
  }
  Signature::sign(consumer_client.get(), metadata);

  wrapPopMessageRequest(metadata, request);
  syncIdleState();
  SPDLOG_DEBUG("Try to pop message from {}", message_queue_.simpleName());
  client_manager_->receiveMessage(message_queue_.serviceAddress(), metadata, request,
                                  absl::ToChronoMilliseconds(consumer_client->getLongPollingTimeout()),
                                  receive_callback_);
}

void ProcessQueueImpl::pullMessage() {
  rmq::PullMessageRequest request;
  absl::flat_hash_map<std::string, std::string> metadata;
  wrapPullMessageRequest(metadata, request);
  syncIdleState();
  SPDLOG_DEBUG("Try to pull message from {}", message_queue_.simpleName());

  auto consumer = consumer_.lock();
  auto timeout = consumer->getLongPollingTimeout();

  auto callback = [this](const InvocationContext<PullMessageResponse>* invocation_context) {
    if (!invocation_context) {
      MQException e("Transport layer failure", -1, __FILE__, __LINE__);
      receive_callback_->onException(e);
      return;
    }

    if (invocation_context->status.ok()) {
      if (google::rpc::Code::OK == invocation_context->response.common().status().code()) {
        ReceiveMessageResult result;
        client_manager_->processPullResult(invocation_context->context, invocation_context->response, result,
                                           invocation_context->remote_address);
        receive_callback_->onSuccess(result);
      } else {
        auto status = invocation_context->response.common().status();
        MQException e(status.message(), status.code(), __FILE__, __LINE__);
        receive_callback_->onException(e);
      }
    } else {
      MQException e(invocation_context->status.error_message(), invocation_context->status.error_code(), __FILE__,
                    __LINE__);
      receive_callback_->onException(e);
    }
  };
  client_manager_->pullMessage(message_queue_.serviceAddress(), metadata, request, absl::ToChronoMilliseconds(timeout),
                               callback);
}

bool ProcessQueueImpl::hasPendingMessages() const {
  absl::MutexLock lk(&messages_mtx_);
  return !cached_messages_.empty();
}

void ProcessQueueImpl::cacheMessages(const std::vector<MQMessageExt>& messages) {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return;
  }

  {
    absl::MutexLock messages_lock_guard(&messages_mtx_);
    // TODO: use lock-when semantics
    absl::MutexLock offsets_lock_guard(&offsets_mtx_);
    for (const auto& message : messages) {
      const std::string& msg_id = message.getMsgId();
      if (!filter_expression_.accept(message)) {
        const std::string& topic = message.getTopic();
        auto callback = [topic, msg_id](bool ok) {
          if (ok) {
            SPDLOG_DEBUG("Ack message[Topic={}, MsgId={}] directly as it fails to pass filter expression", topic,
                         msg_id);
          } else {
            SPDLOG_WARN("Failed to ack message[Topic={}, MsgId={}] directly as it fails to pass filter expression",
                        topic, msg_id);
          }
        };
        consumer->ack(message, callback);
        continue;
      }
      cached_messages_.emplace_back(message);
      cached_message_quantity_.fetch_add(1, std::memory_order_relaxed);
      cached_message_memory_.fetch_add(message.getBody().size(), std::memory_order_relaxed);
      if (MessageModel::BROADCASTING == consumer->messageModel()) {
        if (offsets_.size() == 1 && offsets_.begin()->released_) {
          int64_t previously_released = offsets_.begin()->offset_;
          offsets_.erase(OffsetRecord(previously_released));
        }
        offsets_.emplace(message.getQueueOffset());
      }
    }
  }
}

bool ProcessQueueImpl::take(uint32_t batch_size, std::vector<MQMessageExt>& messages) {
  absl::MutexLock lock(&messages_mtx_);
  if (cached_messages_.empty()) {
    return false;
  }

  for (auto it = cached_messages_.begin(); it != cached_messages_.end();) {
    if (0 == batch_size--) {
      break;
    }

    messages.push_back(*it);
    it = cached_messages_.erase(it);
  }
  return !cached_messages_.empty();
}

bool ProcessQueueImpl::committedOffset(int64_t& offset) {
  absl::MutexLock lk(&offsets_mtx_);
  if (offsets_.empty()) {
    return false;
  }
  if (offsets_.begin()->released_) {
    offset = offsets_.begin()->offset_ + 1;
  } else {
    offset = offsets_.begin()->offset_;
  }
  return true;
}

void ProcessQueueImpl::release(uint64_t body_size, int64_t offset) {
  auto consumer = consumer_.lock();
  if (!consumer) {
    return;
  }

  cached_message_quantity_.fetch_sub(1);
  cached_message_memory_.fetch_sub(body_size);

  if (MessageModel::BROADCASTING == consumer->messageModel()) {
    absl::MutexLock lk(&offsets_mtx_);
    if (offsets_.size() > 1) {
      offsets_.erase(OffsetRecord(offset));
    } else {
      assert(offsets_.begin()->offset_ == offset);
      offsets_.erase(OffsetRecord(offset));
      offsets_.emplace(OffsetRecord(offset, true));
    }
  }
}

void ProcessQueueImpl::wrapPopMessageRequest(absl::flat_hash_map<std::string, std::string>& metadata,
                                             rmq::ReceiveMessageRequest& request) {
  std::shared_ptr<PushConsumer> consumer = consumer_.lock();
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
  request.set_batch_size(consumer->receiveBatchSize());

  // Consume policy
  request.set_consume_policy(rmq::ConsumePolicy::RESUME);

  // Set invisible time
  request.mutable_invisible_duration()->set_seconds(
      std::chrono::duration_cast<std::chrono::seconds>(invisible_time_).count());
  auto fraction = invisible_time_ - std::chrono::duration_cast<std::chrono::seconds>(invisible_time_);
  int32_t nano_seconds = static_cast<int32_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(fraction).count());
  request.mutable_invisible_duration()->set_nanos(nano_seconds);
}

void ProcessQueueImpl::wrapPullMessageRequest(absl::flat_hash_map<std::string, std::string>& metadata,
                                              rmq::PullMessageRequest& request) {
  std::shared_ptr<PushConsumer> consumer = consumer_.lock();
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

std::weak_ptr<PushConsumer> ProcessQueueImpl::getConsumer() { return consumer_; }

std::shared_ptr<ClientManager> ProcessQueueImpl::getClientManager() { return client_manager_; }

MQMessageQueue ProcessQueueImpl::getMQMessageQueue() { return message_queue_; }

const FilterExpression& ProcessQueueImpl::getFilterExpression() const { return filter_expression_; }

ROCKETMQ_NAMESPACE_END