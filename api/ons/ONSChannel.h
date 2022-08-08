#pragma once

#include <cstdint>

#include "ONSClient.h"

ONS_NAMESPACE_BEGIN

enum class ONSChannel : uint8_t
{
  CLOUD = 0,
  ALIYUN = 1,
  ALL = 2,
  LOCAL = 3,
  INNER = 4,
};

ONS_NAMESPACE_END