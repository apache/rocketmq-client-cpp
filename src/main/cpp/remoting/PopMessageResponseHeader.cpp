#include "PopMessageResponseHeader.h"
#include "rocketmq/RocketMQ.h"

#include "absl/strings/numbers.h"
#include "spdlog/spdlog.h"

ROCKETMQ_NAMESPACE_BEGIN

PopMessageResponseHeader* PopMessageResponseHeader::decode(const google::protobuf::Value& root) {
  auto header = new PopMessageResponseHeader();
  auto fields = root.struct_value().fields();
  assign(fields, "popTime", &header->pop_time_);
  assign(fields, "invisibleTime", &header->invisible_time_);
  assign(fields, "reviveQid", &header->revive_queue_id_);
  assign(fields, "restNum", &header->rest_number_);
  assign(fields, "startOffsetInfo", &header->start_offset_info_);
  assign(fields, "msgOffsetInfo", &header->message_offset_info_);
  assign(fields, "orderCountInfo", &header->order_count_info_);
  return header;
}

ROCKETMQ_NAMESPACE_END