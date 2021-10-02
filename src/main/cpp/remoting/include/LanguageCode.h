#pragma once

#include <cstdint>

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum class LanguageCode : std::uint8_t
{
  JAVA = 0,
  CPP = 1,
  DOTNET = 2,
  PYTHON = 3,
  DELPHI = 4,
  ERLANG = 5,
  RUBY = 6,
  OTHER = 7,
  HTTP = 8,
  GO = 9,
  PHP = 10,
  OMS = 11,
};

ROCKETMQ_NAMESPACE_END
