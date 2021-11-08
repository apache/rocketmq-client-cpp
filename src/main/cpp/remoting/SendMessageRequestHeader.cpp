#include "SendMessageRequestHeader.h"

ROCKETMQ_NAMESPACE_BEGIN

template <>
void SendMessageRequestHeader::addEntry(google::protobuf::Map<std::basic_string<char>, google::protobuf::Value>* fields,
                                        absl::string_view key, std::string value) const {
  google::protobuf::Value item;
  item.set_string_value(std::move(value));
  fields->insert({std::string(key.data(), key.length()), item});
}

template <>
void SendMessageRequestHeader::addEntry(google::protobuf::Map<std::basic_string<char>, google::protobuf::Value>* fields,
                                        absl::string_view key, bool value) const {
  google::protobuf::Value item;
  item.set_bool_value(value);
  fields->insert({std::string(key.data(), key.length()), item});
}

void SendMessageRequestHeader::encode(google::protobuf::Value& root) const {
  auto fields = root.mutable_struct_value()->mutable_fields();
  addEntry(fields, "producerGroup", producer_group_);
  addEntry(fields, "topic", topic_);
  addEntry(fields, "defaultTopic", default_topic_);
  addEntry(fields, "defaultTopicQueueNums", default_topic_queue_number_);
  addEntry(fields, "queueId", queue_id_);
  addEntry(fields, "sysFlag", system_flag_);
  addEntry(fields, "bornTimestamp", born_timestamp_);
  addEntry(fields, "flag", flag_);
  addEntry(fields, "properties", properties_);
  addEntry(fields, "reconsumeTimes", reconsume_times_);
  addEntry(fields, "unitMode", unit_mode_);
  addEntry(fields, "batch", batch_);
}

ROCKETMQ_NAMESPACE_END