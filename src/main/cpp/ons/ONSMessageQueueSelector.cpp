#include "ONSMessageQueueSelector.h"

#include <cassert>
#include <cstddef>
#include <functional>
#include <limits>

#include "ons/ONSClientException.h"
#include "ons/ONSErrorCode.h"
#include "rocketmq/MQMessageQueue.h"

ONS_NAMESPACE_BEGIN

ROCKETMQ_NAMESPACE::MQMessageQueue ONSMessageQueueSelector::select(const std::vector<rocketmq::MQMessageQueue>& mqs,
                                                                   const rocketmq::MQMessage& msg, void* arg) {
  if (mqs.empty()) {
    ons::ONSClientException e("Message queue to select from is empty", MESSAGE_SELECTOR_QUEUE_EMPTY);
    throw e;
  }

  std::string* message_group = static_cast<std::string*>(arg);

  std::hash<std::string> hasher;
  std::size_t hash = hasher(*message_group);
  hash = std::numeric_limits<std::size_t>().max() & hash;
  assert(hash >= 0);

  std::size_t remainder = hash % mqs.size();
  assert(remainder >= 0 && remainder < mqs.size());
  return mqs[remainder];
}

ONS_NAMESPACE_END