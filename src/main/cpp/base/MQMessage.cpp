#include "rocketmq/MQMessage.h"
#include "MessageAccessor.h"
#include "Protocol.h"
#include "rocketmq/MQMessageExt.h"
#include <map>
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

class MessageImpl {
private:
  Resource topic_;
  std::map<std::string, std::string> user_attribute_map_;
  SystemAttribute system_attribute_;
  std::string body_;
  friend class MQMessage;
  friend class MQMessageExt;
  friend class MessageAccessor;
};

MQMessage::MQMessage() : MQMessage("", "", "", "") {}

MQMessage::MQMessage(const std::string& topic, const std::string& body) : MQMessage(topic, "", "", body) {}

MQMessage::MQMessage(const std::string& topic, const std::string& tags, const std::string& body)
    : MQMessage(topic, tags, "", body) {}

MQMessage::MQMessage(const std::string& topic, const std::string& tags, const std::string& keys,
                     const std::string& body)
    : impl_(new MessageImpl) {
  impl_->topic_.name = topic;
  impl_->system_attribute_.tag = tags;
  impl_->system_attribute_.keys.emplace_back(keys);
  impl_->body_.clear();
  impl_->body_.reserve(body.length());
  impl_->body_.append(body.data(), body.length());
}

MQMessage::~MQMessage() { delete impl_; }

MQMessage::MQMessage(const MQMessage& other) { impl_ = new MessageImpl(*other.impl_); }

MQMessage& MQMessage::operator=(const MQMessage& other) {
  if (this == &other) {
    return *this;
  }
  *impl_ = *(other.impl_);
  return *this;
}

void MQMessage::setProperty(const std::string& name, const std::string& value) {
  impl_->user_attribute_map_[name] = value;
}

std::string MQMessage::getProperty(const std::string& name) const {
  auto it = impl_->user_attribute_map_.find(name);
  if (impl_->user_attribute_map_.end() == it) {
    return "";
  }
  return it->second;
}

const std::string& MQMessage::getTopic() const { return impl_->topic_.name; }

void MQMessage::setTopic(const std::string& topic) { impl_->topic_.name = topic; }

void MQMessage::setTopic(const char* data, int len) { impl_->topic_.name = std::string(data, len); }

std::string MQMessage::getTags() const { return impl_->system_attribute_.tag; }

void MQMessage::setTags(const std::string& tags) { impl_->system_attribute_.tag = tags; }

const std::vector<std::string>& MQMessage::getKeys() const { return impl_->system_attribute_.keys; }

void MQMessage::setKey(const std::string& key) { impl_->system_attribute_.keys.push_back(key); }

void MQMessage::setKeys(const std::vector<std::string>& keys) { impl_->system_attribute_.keys = keys; }

int MQMessage::getDelayTimeLevel() const { return impl_->system_attribute_.delay_level; }

void MQMessage::setDelayTimeLevel(int level) { impl_->system_attribute_.delay_level = level; }

const std::string& MQMessage::getBody() const { return impl_->body_; }

void MQMessage::setBody(const char* body, int len) {
  impl_->body_.clear();
  impl_->body_.reserve(len);
  impl_->body_.append(body, len);
}

void MQMessage::setBody(const std::string& body) { impl_->body_ = body; }

uint32_t MQMessage::bodyLength() const { return impl_->body_.length(); }

const std::map<std::string, std::string>& MQMessage::getProperties() const { return impl_->user_attribute_map_; }

void MQMessage::setProperties(const std::map<std::string, std::string>& properties) {
  for (const auto& it : properties) {
    impl_->user_attribute_map_.insert({it.first, it.second});
  }
}

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

int32_t MQMessageExt::getReconsumeTimes() const { return impl_->system_attribute_.delivery_count; }

const std::string& MQMessageExt::receiptHandle() const { return impl_->system_attribute_.receipt_handle; }

const std::string& MQMessageExt::traceContext() const { return impl_->system_attribute_.trace_context; }

bool MQMessageExt::operator==(const MQMessageExt& other) {
  return impl_->system_attribute_.message_id == other.impl_->system_attribute_.message_id;
}

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

void MessageAccessor::setDeliveryCount(MQMessageExt& message, int32_t delivery_count) {
  message.impl_->system_attribute_.delivery_count = delivery_count;
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
