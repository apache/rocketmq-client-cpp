#pragma once

#include <chrono>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "ONSClient.h"

ONS_NAMESPACE_BEGIN
class SystemPropKey {
public:
  SystemPropKey() = default;

  ~SystemPropKey() = default;

  static const char* TAG;
  static const char* KEY_SEPARATOR;
  static const char* MSGID;
  static const char* RECONSUMETIMES;
  static const char* STARTDELIVERTIME;
};

class ONSUtil;

class ONSCLIENT_API Message {
public:
  Message() = default;

  Message(const std::string& topic, const std::string& body);

  Message(const std::string& topic, const std::string& tag, const std::string& body);

  Message(const std::string& topic, const std::string& tag, const std::string& key, const std::string& body);

  virtual ~Message() = default;

  // Developers may attach application specific key-value pairs to message,
  // which will be accessible at consuming stage.
  void putUserProperty(const std::string& key, const std::string& value);

  // Used to acquire the key-value pair as was put before the message is sent.
  std::string getUserProperty(const std::string& key) const;

  // To put key-value pairs in batch.
  void setUserProperties(const std::map<std::string, std::string>& user_properties);

  // Acquire a copy all application specific key-value pairs.
  std::map<std::string, std::string> getUserProperties() const;

  std::string getTopic() const;
  void setTopic(const std::string& topic);

  std::string getTag() const;
  void setTag(const std::string& tags);

  std::vector<std::string> getKeys() const;
  void attachKey(const std::string& key);

  std::string getMsgID() const;
  void setMsgID(const std::string& message_id);

  std::chrono::system_clock::time_point getStartDeliverTime() const;

  void setStartDeliverTime(std::chrono::system_clock::time_point delivery_timepoint);

  std::string getBody() const;
  void setBody(const std::string& body);

  std::int32_t getReconsumeTimes() const;

  std::chrono::system_clock::time_point getStoreTimestamp() const;

  std::chrono::system_clock::time_point getBornTimestamp() const {
    return born_timestamp_;
  }

  std::string toUserString() const;

  std::int64_t getQueueOffset() const;

protected:
  void setStoreTimestamp(std::chrono::system_clock::time_point store_timestamp);

  void setBornTimestamp(std::chrono::system_clock::time_point born_timestamp) {
    born_timestamp_ = born_timestamp;
  }

  void setQueueOffset(std::int64_t offset);

  void setReconsumeTimes(std::int32_t reconsume_times);

  std::string toString() const;

  friend class ONSUtil;

private:
  std::string topic_;
  std::string tag_;
  std::string body_;
  std::string message_id_;
  std::chrono::system_clock::time_point store_timestamp_{std::chrono::system_clock::now()};
  std::chrono::system_clock::time_point born_timestamp_{std::chrono::system_clock::now()};
  std::chrono::system_clock::time_point delivery_timestamp_{std::chrono::system_clock::now()};
  std::int64_t queue_offset_{0};
  std::map<std::string, std::string> user_properties_;
  std::vector<std::string> keys_;
  std::int32_t reconsume_times_{0};
};

ONS_NAMESPACE_END