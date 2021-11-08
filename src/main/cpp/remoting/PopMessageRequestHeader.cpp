#include "PopMessageRequestHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

template <>
void PopMessageRequestHeader::addEntry(google::protobuf::Map<std::basic_string<char>, google::protobuf::Value>* fields,
                                       absl::string_view key, std::string value) const {
  google::protobuf::Value item;
  item.set_string_value(std::move(value));
  fields->insert({std::string(key.data(), key.length()), item});
}

template <>
void PopMessageRequestHeader::addEntry(google::protobuf::Map<std::basic_string<char>, google::protobuf::Value>* fields,
                                       absl::string_view key, bool value) const {
  google::protobuf::Value item;
  item.set_bool_value(value);
  fields->insert({std::string(key.data(), key.length()), item});
}

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
}

ROCKETMQ_NAMESPACE_END