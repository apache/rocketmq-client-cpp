#pragma once

#include "ConsumeFromWhere.h"
#include "ConsumeType.h"
#include "SubscriptionData.h"
#include "rocketmq/MessageModel.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

struct ConsumerData {
  std::string group_name_;
  ConsumeType consume_type_{ConsumeType::Unset};
  MessageModel message_model_{MessageModel::CLUSTERING};
  ConsumeFromWhere consume_from_where_{ConsumeFromWhere::ConsumeFromLastOffset};
  absl::flat_hash_set<SubscriptionData> subscription_data_set_;
  bool unit_mode_{false};

  void encode(google::protobuf::Struct& root) const;

  friend bool operator==(const ConsumerData& lhs, const ConsumerData& rhs);
  template <typename H>
  friend H AbslHashValue(H h, const ConsumerData& data);
};

bool operator==(const ConsumerData& lhs, const ConsumerData& rhs);
template <typename H>
H AbslHashValue(H h, const ConsumerData& data) {
  return H::combine(std::move(h), data.group_name_, data.consume_type_, data.message_model_, data.consume_from_where_,
                    data.unit_mode_);
}

ROCKETMQ_NAMESPACE_END