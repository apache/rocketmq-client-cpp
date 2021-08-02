#include "rocketmq/MQMessage.h"
#include "MessageImpl.h"
#include "MessageAccessor.h"
#include "rocketmq/MQMessageExt.h"

ROCKETMQ_NAMESPACE_BEGIN

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
ROCKETMQ_NAMESPACE_END
