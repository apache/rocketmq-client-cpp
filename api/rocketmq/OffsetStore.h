#pragma once

#include "MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

class OffsetStore {
public:
  virtual ~OffsetStore() = default;

  virtual void load() = 0;

  virtual void updateOffset(const MQMessageQueue& message_queue, int64_t offset) = 0;

  virtual bool readOffset(const MQMessageQueue& message_queue, int64_t& offset) = 0;
};

ROCKETMQ_NAMESPACE_END