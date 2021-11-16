#include "HeartbeatData.h"
#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"
#include <google/protobuf/struct.pb.h>

ROCKETMQ_NAMESPACE_BEGIN

void HeartbeatData::encode(google::protobuf::Struct& root) const {
  auto fields = root.mutable_fields();
  addEntry(fields, "clientID", client_id_);

  google::protobuf::Value consumer_data_set;

  for (auto& item : consumer_data_set_) {
    auto node = new google::protobuf::Value;
    item.encode(*node->mutable_struct_value());
    consumer_data_set.mutable_list_value()->mutable_values()->AddAllocated(node);
  }
  fields->insert({"consumerDataSet", consumer_data_set});
}

ROCKETMQ_NAMESPACE_END