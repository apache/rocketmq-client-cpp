#include "MessageGroupQueueSelector.h"

#include <utility>
#include <cassert>

ROCKETMQ_NAMESPACE_BEGIN

MessageGroupQueueSelector::MessageGroupQueueSelector(std::string message_group)
    : message_group_(std::move(message_group)) {}

MQMessageQueue MessageGroupQueueSelector::select(const std::vector<MQMessageQueue>& mqs, const MQMessage& msg,
                                                 void* arg) {
  std::size_t hash_code = std::hash<std::string>{}(message_group_);
  assert(!mqs.empty());
  std::size_t len = mqs.size();
  return mqs[hash_code % len];
}

ROCKETMQ_NAMESPACE_END