#pragma once

#include <cstdint>

#include "ONSClient.h"

ONS_NAMESPACE_BEGIN

enum class Trace : std::uint8_t
{
  ON = 0,
  OFF = 1,
};

ONS_NAMESPACE_END