#include "QueryRouteRequestHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

void QueryRouteRequestHeader::encode(google::protobuf::Value& root) const {
  auto fields = root.mutable_struct_value()->mutable_fields();
  google::protobuf::Value topic;
  topic.set_string_value(topic_);
  fields->insert({"topic", topic});
}

ROCKETMQ_NAMESPACE_END