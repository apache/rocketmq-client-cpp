#pragma once

#include <cstdint>

#include "ONSClient.h"

ONS_NAMESPACE_BEGIN

enum class MessageModel : std::uint8_t
{
  CLUSTERING = 0,
  BROADCASTING = 1,
};

ONS_NAMESPACE_END