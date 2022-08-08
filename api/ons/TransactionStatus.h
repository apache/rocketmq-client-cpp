#pragma once

#include <cstdint>

#include "ONSClient.h"

ONS_NAMESPACE_BEGIN

enum class TransactionStatus : std::uint8_t
{
  CommitTransaction = 0,
  RollbackTransaction = 1,
  Unknow = 2,
};

ONS_NAMESPACE_END