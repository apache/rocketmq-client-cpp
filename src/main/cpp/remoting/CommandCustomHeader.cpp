#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

bool assign(const google::protobuf::Map<std::string, google::protobuf::Value>& fields, const char* label,
            std::string* ptr) {
  if (fields.contains(label)) {
    auto& item = fields.at(label);
    ptr->clear();
    ptr->assign(item.string_value());
    return true;
  }
  return false;
}

template <>
bool assign<bool>(const google::protobuf::Map<std::string, google::protobuf::Value>& fields, const char* label,
                  bool* ptr) {
  if (fields.contains(label)) {
    auto& item = fields.at(label);
    if (item.has_bool_value()) {
      *ptr = item.bool_value();
      return true;
    } else if (item.has_string_value()) {
      *ptr = ("true" == item.string_value());
      return true;
    }
  }
  return false;
}

void addEntry(google::protobuf::Map<std::string, google::protobuf::Value>* fields, absl::string_view key,
              std::string value) {
  google::protobuf::Value item;
  item.set_string_value(std::move(value));
  fields->insert({std::string(key.data(), key.length()), item});
}

template <>
void addEntry<bool>(google::protobuf::Map<std::string, google::protobuf::Value>* fields, absl::string_view key,
                    bool value) {
  google::protobuf::Value item;
  item.set_bool_value(value);
  fields->insert({std::string(key.data(), key.length()), item});
}

ROCKETMQ_NAMESPACE_END