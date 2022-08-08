#pragma once

#include "Message.h"
#include "TransactionStatus.h"

ONS_NAMESPACE_BEGIN
class LocalTransactionExecutor {
public:
  virtual ~LocalTransactionExecutor() = default;

  virtual TransactionStatus execute(const Message& msg) noexcept = 0;
};

// Keep API compatible
using LocalTransactionExecuter = LocalTransactionExecutor;

ONS_NAMESPACE_END