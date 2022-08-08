#pragma once

#include "Message.h"
#include "TransactionStatus.h"

ONS_NAMESPACE_BEGIN
class LocalTransactionChecker {
public:
  virtual ~LocalTransactionChecker() = default;

  virtual TransactionStatus check(const Message& msg) noexcept = 0;
};

ONS_NAMESPACE_END