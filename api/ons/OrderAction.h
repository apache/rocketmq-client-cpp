#pragma once

#include "ONSClient.h"

ONS_NAMESPACE_BEGIN

// order consuming result
enum class OrderAction
{
  // consume success, application could continue to consume next message
  Success,
  // consume fail, suspends the current queue
  Suspend,
};

ONS_NAMESPACE_END