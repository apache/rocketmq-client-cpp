#pragma once

#include "Protocol.h"

ROCKETMQ_NAMESPACE_BEGIN

class MessageImpl {
protected:
  Resource topic_;
  std::map<std::string, std::string> user_attribute_map_;
  SystemAttribute system_attribute_;
  std::string body_;
  friend class MQMessage;
  friend class MQMessageExt;
  friend class MessageAccessor;
};

ROCKETMQ_NAMESPACE_END