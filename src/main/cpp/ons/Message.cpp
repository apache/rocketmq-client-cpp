#include "ons/Message.h"

#include <chrono>
#include <cstdint>
#include <string>

#include "absl/strings/numbers.h"
#include "absl/strings/str_join.h"
#include "absl/time/time.h"

ONS_NAMESPACE_BEGIN

const char* SystemPropKey::TAG = "__TAG";
const char* SystemPropKey::KEY_SEPARATOR = " ";
const char* SystemPropKey::MSGID = "__MSGID";
const char* SystemPropKey::RECONSUMETIMES = "__RECONSUMETIMES";
const char* SystemPropKey::STARTDELIVERTIME = "__STARTDELIVERTIME";

Message::Message(const std::string& topic, const std::string& body) {
  topic_ = std::string(topic.data(), topic.length());
  body_ = std::string(body.data(), body.length());
}

Message::Message(const std::string& topic, const std::string& tag, const std::string& body) : Message(topic, body) {
  if (!tag.empty()) {
    setTag(tag);
  }
}

Message::Message(const std::string& topic, const std::string& tag, const std::string& key, const std::string& body)
    : Message(topic, tag, body) {
  if (!key.empty()) {
    attachKey(key);
  }
}

void Message::putUserProperty(const std::string& key, const std::string& value) {
  std::string k(key.data(), key.length());
  std::string v(value.data(), value.length());

  auto search = user_properties_.find(k);
  if (user_properties_.end() != search) {
    if (search->second == v) {
      return;
    }
    user_properties_.erase(search);
  }
  user_properties_.insert({k, v});
}

std::string Message::getUserProperty(const std::string& key) const {
  std::string k(key.data(), key.length());
  auto it = user_properties_.find(k);
  if (user_properties_.end() != it) {
    return it->second;
  }
  return std::string();
}

void Message::setUserProperties(const std::map<std::string, std::string>& user_properties) {
  for (const auto& it : user_properties) {
    putUserProperty(it.first, it.second);
  }
}

std::map<std::string, std::string> Message::getUserProperties() const {
  return user_properties_;
}

std::string Message::getTopic() const {
  return topic_;
}

void Message::setTopic(const std::string& topic) {
  if (topic.empty()) {
    return;
  }
  topic_ = std::string(topic.data(), topic.length());
}

std::string Message::getTag() const {
  return tag_;
}

void Message::setTag(const std::string& tag) {
  if (tag.empty()) {
    return;
  }

  tag_ = std::string(tag.data(), tag.length());
}

std::string Message::getMsgID() const {
  return message_id_;
}

void Message::setMsgID(const std::string& message_id) {
  if (message_id.empty()) {
    return;
  }
  message_id_ = std::string(message_id.data(), message_id.length());
}

std::vector<std::string> Message::getKeys() const {
  return keys_;
}

void Message::attachKey(const std::string& key) {
  if (key.empty()) {
    return;
  }

  keys_.push_back(std::string(key.data(), key.length()));
}

std::chrono::system_clock::time_point Message::getStartDeliverTime() const {
  return delivery_timestamp_;
}

void Message::setStartDeliverTime(std::chrono::system_clock::time_point delivery_timepoint) {
  delivery_timestamp_ = delivery_timepoint;
}

std::string Message::getBody() const {
  return body_;
}

void Message::setBody(const std::string& body) {
  if (body.empty()) {
    body_.clear();
    return;
  }
  body_ = std::string(body.data(), body.length());
}

std::int32_t Message::getReconsumeTimes() const {
  return reconsume_times_;
}

void Message::setReconsumeTimes(std::int32_t reconsume_times) {
  reconsume_times_ = reconsume_times;
}

std::chrono::system_clock::time_point Message::getStoreTimestamp() const {
  return store_timestamp_;
}

void Message::setStoreTimestamp(std::chrono::system_clock::time_point store_timepoint) {
  store_timestamp_ = store_timepoint;
}

std::int64_t Message::getQueueOffset() const {
  return queue_offset_;
}

void Message::setQueueOffset(std::int64_t queue_offset) {
  queue_offset_ = queue_offset;
}

std::string Message::toString() const {
  std::stringstream ss;
  ss << "Message [topic=" << topic_ << ", body=" << body_ << "]";
  return ss.str();
}

std::string Message::toUserString() const {
  return absl::StrJoin(user_properties_, ",", absl::PairFormatter("="));
}

ONS_NAMESPACE_END