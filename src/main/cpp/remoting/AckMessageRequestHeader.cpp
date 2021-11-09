#include "AckMessageRequestHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

void AckMessageRequestHeader::encode(google::protobuf::Value& root) const {
  auto fields = root.mutable_struct_value()->mutable_fields();
  addEntry(fields, "consumerGroup", consumer_group_);
  addEntry(fields, "topic", topic_);
  addEntry(fields, "queueId", queue_id_);
  addEntry(fields, "extraInfo", extra_info_);
  addEntry(fields, "offset", offset_);
}

ROCKETMQ_NAMESPACE_END