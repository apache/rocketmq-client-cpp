#include "QueryConsumerOffsetResponseHeader.h"
#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

QueryConsumerOffsetResponseHeader* QueryConsumerOffsetResponseHeader::decode(const google::protobuf::Value& root) {
  auto header = new QueryConsumerOffsetResponseHeader();
  auto fields = root.struct_value().fields();
  assign(fields, "offset", &header->offset_);
  return header;
}

ROCKETMQ_NAMESPACE_END