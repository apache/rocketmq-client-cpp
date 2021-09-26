#include "PullConsumerImpl.h"
#include "ClientManagerFactory.h"
#include "InvocationContext.h"
#include "Signature.h"
#include "apache/rocketmq/v1/definition.pb.h"
#include "rocketmq/ErrorCode.h"
#include "rocketmq/MQClientException.h"
#include "rocketmq/MessageModel.h"
#include <exception>
#include <system_error>

ROCKETMQ_NAMESPACE_BEGIN

void PullConsumerImpl::start() {
  ClientImpl::start();
  if (State::STARTED != state_.load(std::memory_order_relaxed)) {
    SPDLOG_WARN("Unexpected state: {}", state_.load(std::memory_order_relaxed));
    return;
  }
  client_manager_->addClientObserver(shared_from_this());
}

void PullConsumerImpl::shutdown() {
  // Shutdown services started by current tier

  // Shutdown services that are started by the parent
  ClientImpl::shutdown();
  State expected = State::STOPPING;
  if (state_.compare_exchange_strong(expected, State::STOPPED)) {
    SPDLOG_INFO("DefaultMQPullConsumerImpl stopped");
  }
}

std::future<std::vector<MQMessageQueue>> PullConsumerImpl::queuesFor(const std::string& topic) {
  auto promise = std::make_shared<std::promise<std::vector<MQMessageQueue>>>();
  {
    absl::MutexLock lk(&topic_route_table_mtx_);
    if (topic_route_table_.contains(topic)) {
      TopicRouteDataPtr topic_route = topic_route_table_.at(topic);
      auto partitions = topic_route->partitions();
      std::vector<MQMessageQueue> message_queues;
      message_queues.reserve(partitions.size());
      for (const auto& partition : partitions) {
        message_queues.emplace_back(partition.asMessageQueue());
      }
      promise->set_value(std::move(message_queues));
      return promise->get_future();
    }
  }

  auto callback = [promise](const std::error_code& ec, const TopicRouteDataPtr& route) {
    if (ec) {
      MQClientException e(ec.message(), ec.value(), __FILE__, __LINE__);
      promise->set_exception(std::make_exception_ptr(e));
      return;
    }

    std::vector<MQMessageQueue> message_queues;
    for (const auto& partition : route->partitions()) {
      message_queues.emplace_back(partition.asMessageQueue());
    }
    promise->set_value(message_queues);
  };

  getRouteFor(topic, callback);
  return promise->get_future();
}

std::future<int64_t> PullConsumerImpl::queryOffset(const OffsetQuery& query) {
  QueryOffsetRequest request;
  switch (query.policy) {
  case QueryOffsetPolicy::BEGINNING:
    request.set_policy(rmq::QueryOffsetPolicy::BEGINNING);
    break;
  case QueryOffsetPolicy::END:
    request.set_policy(rmq::QueryOffsetPolicy::END);
    break;
  case QueryOffsetPolicy::TIME_POINT:
    request.set_policy(rmq::QueryOffsetPolicy::TIME_POINT);
    auto duration = absl::FromChrono(query.time_point.time_since_epoch());
    int64_t seconds = absl::ToInt64Seconds(duration);
    request.mutable_time_point()->set_seconds(seconds);
    request.mutable_time_point()->set_nanos(absl::ToInt64Nanoseconds(duration - absl::Seconds(seconds)));
    break;
  }

  request.mutable_partition()->mutable_topic()->set_name(query.message_queue.getTopic());
  request.mutable_partition()->mutable_topic()->set_resource_namespace(resource_namespace_);

  request.mutable_partition()->set_id(query.message_queue.getQueueId());

  absl::flat_hash_map<std::string, std::string> metadata;

  Signature::sign(this, metadata);

  // TODO: Use std::unique_ptr if C++14 is adopted.
  auto promise_ptr = std::make_shared<std::promise<int64_t>>();
  auto callback = [promise_ptr](const std::error_code& ec, const QueryOffsetResponse& response) {
    if (ec) {
      MQClientException e(ec.message(), ec.value(), __FILE__, __LINE__);
      promise_ptr->set_exception(std::make_exception_ptr(e));
      return;
    }
    promise_ptr->set_value(response.offset());
  };

  client_manager_->queryOffset(query.message_queue.serviceAddress(), metadata, request,
                               absl::ToChronoMilliseconds(io_timeout_), callback);
  return promise_ptr->get_future();
}

void PullConsumerImpl::pull(const PullMessageQuery& query, PullCallback* cb) {
  PullMessageRequest request;
  request.set_offset(query.offset);
  auto duration = absl::FromChrono(query.await_time);
  int64_t seconds = absl::ToInt64Seconds(duration);
  request.mutable_await_time()->set_seconds(seconds);
  request.mutable_await_time()->set_nanos(absl::ToInt64Nanoseconds(duration - absl::Seconds(seconds)));
  request.mutable_group()->set_name(group_name_);
  request.mutable_group()->set_resource_namespace(resource_namespace_);

  request.mutable_partition()->mutable_topic()->set_name(query.message_queue.getTopic());
  request.mutable_partition()->mutable_topic()->set_resource_namespace(resource_namespace_);
  request.mutable_partition()->set_id(query.message_queue.getQueueId());
  request.set_client_id(clientId());

  std::string target_host = query.message_queue.serviceAddress();
  assert(!target_host.empty());

  auto callback = [this, target_host, cb](const InvocationContext<PullMessageResponse>* invocation_context) {
    if (!invocation_context || !invocation_context->status.ok()) {
      std::error_code ec = ErrorCode::RequestTimeout;
      cb->onFailure(ec);
      return;
    }

    auto response = invocation_context->response;
    auto biz_status = response.common().status();

    switch (biz_status.code()) {
    case google::rpc::Code::OK: {
      std::vector<MQMessageExt> messages;
      for (const auto& item : response.messages()) {
        MQMessageExt message_ext;
        if (client_manager_->wrapMessage(item, message_ext)) {
          messages.emplace_back(message_ext);
        }
      }
      PullResult pull_result(response.min_offset(), response.max_offset(), response.next_offset(), std::move(messages));
      cb->onSuccess(pull_result);
    } break;

    case google::rpc::Code::PERMISSION_DENIED: {
      SPDLOG_WARN("PermissionDenied: {}", response.common().status().message());
      std::error_code ec = ErrorCode::Forbidden;
      cb->onFailure(ec);
    } break;

    case google::rpc::Code::UNAUTHENTICATED: {
      SPDLOG_WARN("Unauthenticated: {}", response.common().status().message());
      std::error_code ec = ErrorCode::Unauthorized;
      cb->onFailure(ec);
    } break;

    case google::rpc::Code::DEADLINE_EXCEEDED: {
      SPDLOG_WARN("GatewayTimeout: {}", response.common().status().message());
      std::error_code ec = ErrorCode::GatewayTimeout;
      cb->onFailure(ec);
    } break;

    case google::rpc::Code::INVALID_ARGUMENT: {
      SPDLOG_WARN("BadRequest: {}", response.common().status().message());
      std::error_code ec = ErrorCode::BadRequest;
      cb->onFailure(ec);
    } break;

    case google::rpc::Code::INTERNAL: {
      SPDLOG_WARN("ServerIntervalError: {}", response.common().status().message());
      std::error_code ec = ErrorCode::InternalServerError;
      cb->onFailure(ec);
    } break;

    default: {
      SPDLOG_WARN("Unsupported response code. Please upgrade to lastest SDK");
      std::error_code ec = ErrorCode::NotImplemented;
      cb->onFailure(ec);
    } break;
    }
  };

  absl::flat_hash_map<std::string, std::string> metadata;
  Signature::sign(this, metadata);

  client_manager_->pullMessage(target_host, metadata, request, absl::ToChronoMilliseconds(long_polling_timeout_),
                               callback);
}

void PullConsumerImpl::prepareHeartbeatData(HeartbeatRequest& request) {
  request.set_client_id(clientId());
  auto consumer_data = request.mutable_consumer_data();
  consumer_data->mutable_group()->set_resource_namespace(resource_namespace_);
  consumer_data->mutable_group()->set_name(group_name_);
  switch (message_model_) {
  case MessageModel::BROADCASTING:
    consumer_data->set_consume_model(rmq::ConsumeModel::BROADCASTING);
    break;
  case MessageModel::CLUSTERING:
    consumer_data->set_consume_model(rmq::ConsumeModel::CLUSTERING);
    break;
  }
}

ROCKETMQ_NAMESPACE_END