#pragma once

#include "RocketMQ.h"
#include <cstdint>

ROCKETMQ_NAMESPACE_BEGIN

class Transaction {
public:
  Transaction() = default;

  virtual ~Transaction() = default;

  virtual bool commit() = 0;

  virtual bool rollback() = 0;
};

enum class TransactionState : int8_t {
  COMMIT = 0,
  ROLLBACK = 1,
};

ROCKETMQ_NAMESPACE_END