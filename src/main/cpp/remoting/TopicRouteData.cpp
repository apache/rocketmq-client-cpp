#include "TopicRouteData.h"
#include "BrokerData.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

TopicRouteData TopicRouteData::decode(const google::protobuf::Struct& root) {
  auto fields = root.fields();

  TopicRouteData topic_route_data;

  if (fields.contains("queueDatas")) {
    auto queue_data_list = fields.at("queueDatas");

    for (auto& item : queue_data_list.list_value().values()) {
      topic_route_data.queue_data_.push_back(QueueData::decode(item.struct_value()));
    }
  }

  if (fields.contains("brokerDatas")) {
    auto broker_data_list = fields.at("brokerDatas");
    for (auto& item : broker_data_list.list_value().values()) {
      topic_route_data.broker_data_.push_back(BrokerData::decode(item.struct_value()));
    }
  }

  return topic_route_data;
}

ROCKETMQ_NAMESPACE_END