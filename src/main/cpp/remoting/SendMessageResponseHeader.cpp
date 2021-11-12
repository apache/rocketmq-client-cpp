#include "SendMessageResponseHeader.h"

#include "absl/strings/numbers.h"

#include "LoggerImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

SendMessageResponseHeader* SendMessageResponseHeader::decode(const google::protobuf::Value& root) {
  auto header = new SendMessageResponseHeader();

  auto fields = root.struct_value().fields();
  assign(fields, "msgId", &header->message_id_);
  assign(fields, "queueId", &header->queue_id_);
  assign(fields, "queueOffset", &header->queue_offset_);
  assign(fields, "transactionId", &header->transaction_id_);

  return header;
}

ROCKETMQ_NAMESPACE_END