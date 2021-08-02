#include "rocketmq/MQMessageExt.h"
#include "MessageImpl.h"

ROCKETMQ_NAMESPACE_BEGIN

MQMessageExt::MQMessageExt() : MQMessage() {}

MQMessageExt::MQMessageExt(const MQMessageExt& other) : MQMessage(other) {}

MQMessageExt& MQMessageExt::operator=(const MQMessageExt& other) {
  if (this == &other) {
    return *this;
  }

  *impl_ = *(other.impl_);
  return *this;
}

int32_t MQMessageExt::getQueueId() const { return impl_->system_attribute_.partition_id; }

std::chrono::system_clock::time_point MQMessageExt::bornTimestamp() const {
  return absl::ToChronoTime(impl_->system_attribute_.born_timestamp);
}

int64_t MQMessageExt::getBornTimestamp() const { return absl::ToUnixMillis(impl_->system_attribute_.born_timestamp); }

std::string MQMessageExt::getBornHost() const { return impl_->system_attribute_.born_host; }

std::chrono::system_clock::time_point MQMessageExt::storeTimestamp() const {
  return absl::ToChronoTime(impl_->system_attribute_.store_timestamp);
}

int64_t MQMessageExt::getStoreTimestamp() const { return absl::ToUnixMillis(impl_->system_attribute_.store_timestamp); }

std::string MQMessageExt::getStoreHost() const { return impl_->system_attribute_.store_host; }

const std::string& MQMessageExt::getMsgId() const { return impl_->system_attribute_.message_id; }

int64_t MQMessageExt::getQueueOffset() const { return impl_->system_attribute_.partition_offset; }

int32_t MQMessageExt::getDeliveryAttempt() const { return impl_->system_attribute_.attempt_times; }

const std::string& MQMessageExt::receiptHandle() const { return impl_->system_attribute_.receipt_handle; }

const std::string& MQMessageExt::traceContext() const { return impl_->system_attribute_.trace_context; }

bool MQMessageExt::operator==(const MQMessageExt& other) {
  return impl_->system_attribute_.message_id == other.impl_->system_attribute_.message_id;
}

ROCKETMQ_NAMESPACE_END