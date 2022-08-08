#pragma once

#include <iomanip>
#include <sstream>
#include <string>

#include "ONSClient.h"

ONS_NAMESPACE_BEGIN

class MessageQueueONS {
public:
  MessageQueueONS() = default;

  MessageQueueONS(std::string topic, std::string broker_name, int queue_id)
      : topic_(std::move(topic)), broker_name_(std::move(broker_name)), queue_id_(queue_id) {
  }

  std::string getTopic() const;
  void setTopic(const std::string& topic);

  std::string getBrokerName() const;
  void setBrokerName(const std::string& broker_name);

  int getQueueId() const;
  void setQueueId(int queue_id);

  bool operator==(const MessageQueueONS& mq) const;
  bool operator<(const MessageQueueONS& mq) const;
  int compareTo(const MessageQueueONS& mq) const;

private:
  std::string topic_;
  std::string broker_name_;
  int queue_id_{-1};
};

ONS_NAMESPACE_END