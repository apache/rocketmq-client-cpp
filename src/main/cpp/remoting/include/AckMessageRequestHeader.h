#pragma once

#include <cstdint>

#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class AckMessageRequestHeader : public CommandCustomHeader {
public:
  void encode(google::protobuf::Value& root) const override;

protected:
  template <typename T, typename = typename std::enable_if<std::is_integral<T>::value, void>>
  void addEntry(google::protobuf::Map<std::basic_string<char>, google::protobuf::Value>* fields, absl::string_view key,
                T value) const {
    google::protobuf::Value item;
    item.set_number_value(value);
    fields->insert({std::string(key.data(), key.size()), item});
  }

private:
  std::string consumer_group_;
  std::string topic_;
  std::int32_t queue_id_{0};
  std::string extra_info_;
  std::int64_t offset_{0};
};

ROCKETMQ_NAMESPACE_END