#include "PopMessageRequestHeader.h"
#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

void PopMessageRequestHeader::encode(google::protobuf::Value& root) const {
  auto fields = root.mutable_struct_value()->mutable_fields();
  addEntry(fields, "consumerGroup", consumer_group_);
  addEntry(fields, "topic", topic_);
  addEntry(fields, "queueId", queue_id_);
  addEntry(fields, "maxMsgNums", max_message_number_);
  addEntry(fields, "invisibleTime", invisible_time_);
  addEntry(fields, "pollTime", poll_time_);
  addEntry(fields, "bornTime", born_time_);
  addEntry(fields, "initMode", init_mode_);
  addEntry(fields, "queueIdListString", queue_id_list_string_);
  addEntry(fields, "expType", expression_type_);
  addEntry(fields, "exp", expression_);
  addEntry(fields, "order", order_);
  addEntry(fields, "generateMessageHandle", generate_message_handle_);
}

ROCKETMQ_NAMESPACE_END