#include "SubscriptionData.h"

#include <cstdint>

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

bool operator==(const SubscriptionData& lhs, const SubscriptionData& rhs) {
  return lhs.class_filter_mode_ == rhs.class_filter_mode_ && lhs.topic_ == rhs.topic_ &&
         lhs.sub_string_ == rhs.sub_string_ && lhs.sub_version_ == rhs.sub_version_;
}

void SubscriptionData::encode(google::protobuf::Struct& root) const {
  auto fields = root.mutable_fields();

  addEntry(fields, "classFilterMode", class_filter_mode_);
  addEntry(fields, "topic", topic_);
  addEntry(fields, "subString", sub_string_);
  addEntry(fields, "subVersion", sub_version_);
}

ROCKETMQ_NAMESPACE_END