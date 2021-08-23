#pragma once

#include "rocketmq/MQMessageExt.h"
#include "rocketmq/RocketMQ.h"

#include "Protocol.h"

ROCKETMQ_NAMESPACE_BEGIN

class MessageAccessor {

public:
  static void setMessageId(MQMessageExt& message, std::string message_id);

  static void setBornTimestamp(MQMessageExt& message, absl::Time born_timestamp);

  static void setStoreTimestamp(MQMessageExt& message, absl::Time store_timestamp);

  static void setQueueId(MQMessageExt& message, int32_t queue_id);

  static void setQueueOffset(MQMessageExt& message, int64_t queue_offset);

  static void setBornHost(MQMessageExt& message, std::string born_host);

  static void setStoreHost(MQMessageExt& message, std::string store_host);

  static void setDeliveryTimestamp(MQMessageExt& message, absl::Time delivery_timestamp);

  static void setDeliveryAttempt(MQMessageExt& message, int32_t attempt_times);

  static void setDecodedTimestamp(MQMessageExt &message, absl::Time decode_timestamp);
  static absl::Time decodedTimestamp(const MQMessageExt &message);
  
  static void setInvisiblePeriod(MQMessageExt& message, absl::Duration invisible_period);

  static void setReceiptHandle(MQMessageExt& message, std::string receipt_handle);
  static void setTraceContext(MQMessageExt& message, std::string trace_context);

  static void setMessageType(MQMessage& message, MessageType message_type);

  static void setTargetEndpoint(MQMessage& message, const std::string& target_endpoint);

  static const std::string& targetEndpoint(const MQMessage& message);
};

ROCKETMQ_NAMESPACE_END