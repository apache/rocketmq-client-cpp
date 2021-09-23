#pragma once

#include "rocketmq/MQMessage.h"

ROCKETMQ_NAMESPACE_BEGIN

class OrphanTransactionCallback {
public:
  virtual ~OrphanTransactionCallback() = default;

  virtual void onOrphanTransaction(const MQMessage& message) = 0;
};

ROCKETMQ_NAMESPACE_END