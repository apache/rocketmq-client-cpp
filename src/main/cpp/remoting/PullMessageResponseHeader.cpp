#include "PullMessageResponseHeader.h"
#include "CommandCustomHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

PullMessageResponseHeader* PullMessageResponseHeader::decode(const google::protobuf::Value& root) {
  auto header = new PullMessageResponseHeader();
  auto fields = root.struct_value().fields();
  assign(fields, "suggestWhichBrokerId", &header->suggest_which_broker_id_);
  assign(fields, "nextBeginOffset", &header->next_begin_offset_);
  assign(fields, "minOffset", &header->min_offset_);
  assign(fields, "maxOffset", &header->max_offset_);
  assign(fields, "topicSysFlag", &header->topic_system_flag_);
  assign(fields, "groupSysFlag", &header->group_system_flag_);
  return header;
}

ROCKETMQ_NAMESPACE_END