#pragma once

#include <cstdint>

#include "absl/container/flat_hash_set.h"
#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "google/protobuf/struct.pb.h"

#include "rocketmq/MQMessageQueue.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

struct SubscriptionData {
  bool class_filter_mode_{false};
  std::string topic_;
  std::string sub_string_;
  std::int64_t sub_version_{absl::ToInt64Milliseconds(absl::Now() - absl::UnixEpoch())};

  void encode(google::protobuf::Struct& root) const;

  friend bool operator==(const SubscriptionData& lhs, const SubscriptionData& rhs);
  template <typename H>
  friend H AbslHashValue(H h, const SubscriptionData& data);
};

bool operator==(const SubscriptionData& lhs, const SubscriptionData& rhs);
template <typename H>
H AbslHashValue(H h, const SubscriptionData& data) {
  return H::combine(std::move(h), data.class_filter_mode_, data.topic_, data.sub_string_, data.sub_version_);
}

ROCKETMQ_NAMESPACE_END