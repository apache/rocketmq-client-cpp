#pragma once

#include "RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

class Transaction {
public:
  Transaction() = default;

  virtual ~Transaction() = default;

  virtual bool commit() = 0;

  virtual bool rollback() = 0;
};

ROCKETMQ_NAMESPACE_END