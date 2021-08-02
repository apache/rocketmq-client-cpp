#include "MessageAccessor.h"
#include "MessageImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

void MessageAccessor::setMessageId(MQMessageExt& message, std::string message_id) {
  message.impl_->system_attribute_.message_id = std::move(message_id);
}

void MessageAccessor::setBornTimestamp(MQMessageExt& message, absl::Time born_timestamp) {
  message.impl_->system_attribute_.born_timestamp = born_timestamp;
}

void MessageAccessor::setStoreTimestamp(MQMessageExt& message, absl::Time store_timestamp) {
  message.impl_->system_attribute_.store_timestamp = store_timestamp;
}

void MessageAccessor::setQueueId(MQMessageExt& message, int32_t queue_id) {
  message.impl_->system_attribute_.partition_id = queue_id;
}

void MessageAccessor::setQueueOffset(MQMessageExt& message, int64_t queue_offset) {
  message.impl_->system_attribute_.partition_offset = queue_offset;
}

void MessageAccessor::setBornHost(MQMessageExt& message, std::string born_host) {
  message.impl_->system_attribute_.born_host = std::move(born_host);
}

void MessageAccessor::setStoreHost(MQMessageExt& message, std::string store_host) {
  message.impl_->system_attribute_.store_host = std::move(store_host);
}

void MessageAccessor::setDeliveryTimestamp(MQMessageExt& message, absl::Time delivery_timestamp) {
  message.impl_->system_attribute_.delivery_timestamp = delivery_timestamp;
}

void MessageAccessor::setDeliveryAttempt(MQMessageExt& message, int32_t attempt_times) {
  message.impl_->system_attribute_.attempt_times = attempt_times;
}

void MessageAccessor::setDecodedTimestamp(MQMessageExt& message, absl::Time decode_timestamp) {
  message.impl_->system_attribute_.decode_timestamp = decode_timestamp;
}

void MessageAccessor::setInvisiblePeriod(MQMessageExt& message, absl::Duration invisible_period) {
  message.impl_->system_attribute_.invisible_period = invisible_period;
}

void MessageAccessor::setReceiptHandle(MQMessageExt& message, std::string receipt_handle) {
  message.impl_->system_attribute_.receipt_handle = std::move(receipt_handle);
}

void MessageAccessor::setTraceContext(MQMessageExt& message, std::string trace_context) {
  message.impl_->system_attribute_.trace_context = std::move(trace_context);
}

void MessageAccessor::setMessageType(MQMessage& message, MessageType message_type) {
  message.impl_->system_attribute_.message_type = message_type;
}

void MessageAccessor::setTargetEndpoint(MQMessage& message, const std::string& target_endpoint) {
  message.impl_->system_attribute_.target_endpoint = target_endpoint;
}

const std::string& MessageAccessor::targetEndpoint(const MQMessage& message) {
  return message.impl_->system_attribute_.target_endpoint;
}

ROCKETMQ_NAMESPACE_END