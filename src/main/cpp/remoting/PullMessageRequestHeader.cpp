#include "PullMessageRequestHeader.h"
#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

void PullMessageRequestHeader::encode(google::protobuf::Value& root) const {
  auto fields = root.mutable_struct_value()->mutable_fields();
  addEntry(fields, "consumerGroup", consumer_group_);
  addEntry(fields, "topic", topic_);
  addEntry(fields, "queueId", queue_id_);
  addEntry(fields, "queueOffset", queue_offset_);
  addEntry(fields, "maxMsgNums", max_msg_number_);
  addEntry(fields, "sysFlag", system_flag_);
  addEntry(fields, "commitOffset", commit_offset_);
  addEntry(fields, "suspendTimeoutMillis", suspend_timeout_millis_);
  addEntry(fields, "subscription", subscription_);
  addEntry(fields, "subVersion", sub_version_);
  addEntry(fields, "expressionType", expression_type_);
  addEntry(fields, "subProperties", sub_properties_);
  addEntry(fields, "brokerName", broker_name_);
}

ROCKETMQ_NAMESPACE_END