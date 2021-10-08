#include "BrokerData.h"
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

BrokerData BrokerData::decode(const google::protobuf::Struct& root) {
  BrokerData broker_data;
  auto fields = root.fields();
  if (fields.contains("cluster")) {
    broker_data.cluster_ = fields["cluster"].string_value();
  }

  if (fields.contains("brokerName")) {
    broker_data.broker_name_ = fields["brokerName"].string_value();
  }

  if (fields.contains("brokerAddrs")) {
    auto items = fields["brokerAddrs"].struct_value().fields();
    for (const auto& item : items) {
      auto k = std::stoll(item.first);
      broker_data.broker_addresses_.insert({k, item.second.string_value()});
    }
  }
  return broker_data;
}

ROCKETMQ_NAMESPACE_END