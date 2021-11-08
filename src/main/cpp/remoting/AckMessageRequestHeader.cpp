#include "AckMessageRequestHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

template <>
void AckMessageRequestHeader::addEntry(google::protobuf::Map<std::basic_string<char>, google::protobuf::Value>* fields,
                                       absl::string_view key, std::string value) const {
  google::protobuf::Value item;
  item.set_string_value(std::move(value));
  fields->insert({std::string(key.data(), key.length()), item});
}

template <>
void AckMessageRequestHeader::addEntry(google::protobuf::Map<std::basic_string<char>, google::protobuf::Value>* fields,
                                       absl::string_view key, bool value) const {
  google::protobuf::Value item;
  item.set_bool_value(value);
  fields->insert({std::string(key.data(), key.length()), item});
}

void AckMessageRequestHeader::encode(google::protobuf::Value& root) const {
  auto fields = root.mutable_struct_value()->mutable_fields();
  addEntry(fields, "consumerGroup", consumer_group_);
  addEntry(fields, "topic", topic_);
  addEntry(fields, "queueId", queue_id_);
  addEntry(fields, "extraInfo", extra_info_);
  addEntry(fields, "offset", offset_);
}

ROCKETMQ_NAMESPACE_END