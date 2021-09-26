#pragma once

#include <chrono>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/strings/string_view.h"

#include "MQMessageQueue.h"
#include "MessageType.h"

ROCKETMQ_NAMESPACE_BEGIN

class MessageImpl;

class MessageAccessor;

class MQMessage {
public:
  MQMessage();
  MQMessage(const std::string &topic, const std::string &body);
  MQMessage(const std::string &topic, const std::string &tags,
            const std::string &body);
  MQMessage(const std::string &topic, const std::string &tags,
            const std::string &keys, const std::string &body);

  virtual ~MQMessage();

  MQMessage(const MQMessage &other);
  MQMessage &operator=(const MQMessage &other);

  const std::string &getMsgId() const;

  void setProperty(const std::string &name, const std::string &value);
  std::string getProperty(const std::string &name) const;

  const std::string &getTopic() const;
  void setTopic(const std::string &topic);
  void setTopic(const char *data, int len);

  std::string getTags() const;
  void setTags(const std::string &tags);

  const std::vector<std::string> &getKeys() const;

  /**
   * @brief Add a unique key for the message
   * TODO: a message may be associated with multiple keys. setKey, actually mean
   * attach the given key to the message. Better rename it.
   * @param key Unique key in perspective of bussiness logic.
   */
  void setKey(const std::string &key);
  void setKeys(const std::vector<std::string> &keys);

  int getDelayTimeLevel() const;
  void setDelayTimeLevel(int level);

  const std::string &traceContext() const;
  void traceContext(const std::string &trace_context);

  std::string getBornHost() const;

  std::chrono::system_clock::time_point deliveryTimestamp() const;

  const std::string &getBody() const;
  void setBody(const char *data, int len);
  void setBody(const std::string &body);

  uint32_t bodyLength() const;

  const std::map<std::string, std::string> &getProperties() const;
  void setProperties(const std::map<std::string, std::string> &properties);

  void messageType(MessageType message_type);
  MessageType messageType() const;

  void bindMessageGroup(absl::string_view message_group) {
    message_group_ = std::string(message_group.data(), message_group.length());
  }

  void bindMessageQueue(const MQMessageQueue &message_queue) {
    message_queue_ = message_queue;
  }

  const std::string &messageGroup() const { return message_group_; }

  const MQMessageQueue &messageQueue() const { return message_queue_; }

protected:
  MessageImpl *impl_;

  friend class MessageAccessor;

private:
  std::string message_group_;
  MQMessageQueue message_queue_;
};

ROCKETMQ_NAMESPACE_END