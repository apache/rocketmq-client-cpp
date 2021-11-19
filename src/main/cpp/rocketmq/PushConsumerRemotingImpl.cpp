#include "PushConsumerRemotingImpl.h"
#include "rocketmq/ExpressionType.h"
#include "rocketmq/RocketMQ.h"
#include "rocketmq/State.h"
#include <atomic>

ROCKETMQ_NAMESPACE_BEGIN

void PushConsumerRemotingImpl::start() {
  ClientImpl::start();

  State expected = State::STARTING;

  if (!state_.compare_exchange_strong(expected, State::STARTED, std::memory_order_relaxed)) {
    SPDLOG_WARN("Failed to start, caused by unexpected state: {}", expected);
    return;
  }
}

void PushConsumerRemotingImpl::shutdown() {
  State expected = State::STARTED;
  if (!state_.compare_exchange_strong(expected, State::STOPPING, std::memory_order_relaxed)) {
    SPDLOG_WARN("Failed to shutdown, caused by unexpected state: {}", expected);
    return;
  }

  ClientImpl::shutdown();
}

absl::optional<FilterExpression> PushConsumerRemotingImpl::getFilterExpression(const std::string& topic) const {
  absl::MutexLock lk(&topic_filter_expression_map_mtx_);
  if (topic_filter_expression_map_.contains(topic)) {
    return absl::make_optional(topic_filter_expression_map_.at(topic));
  }
  return absl::optional<FilterExpression>();
}

void PushConsumerRemotingImpl::subscribe(std::string topic, std::string tag) {
  FilterExpression filter_expression(tag);
  {
    absl::MutexLock lk(&topic_filter_expression_map_mtx_);
    topic_filter_expression_map_.insert_or_assign(topic, filter_expression);
  }
}

void PushConsumerRemotingImpl::prepareHeartbeatData(HeartbeatRequest& request) {
  SPDLOG_DEBUG("Preparing heartbeat data");
  request.set_client_id(clientId());
  request.mutable_consumer_data()->mutable_group()->set_name(group_name_);
  request.mutable_consumer_data()->mutable_group()->set_resource_namespace(resource_namespace_);

  auto subscriptions = request.mutable_consumer_data()->mutable_subscriptions();
  {
    absl::MutexLock lk(&topic_filter_expression_map_mtx_);
    for (const auto& item : topic_filter_expression_map_) {
      auto subscription = new rmq::SubscriptionEntry;
      subscription->mutable_topic()->set_name(item.first);
      subscription->mutable_topic()->set_resource_namespace(resourceNamespace());

      switch (item.second.type_) {
        case ExpressionType::TAG: {
          subscription->mutable_expression()->set_type(rmq::FilterType::TAG);
          break;
        }
        case ExpressionType::SQL92: {
          subscription->mutable_expression()->set_type(rmq::FilterType::SQL);
          break;
        }
      }
      subscription->mutable_expression()->set_expression(item.second.content_);
      subscriptions->AddAllocated(subscription);
    }
  }
}

ROCKETMQ_NAMESPACE_END