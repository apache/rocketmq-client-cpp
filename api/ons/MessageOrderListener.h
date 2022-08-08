#pragma once

#include "ConsumeOrderContext.h"
#include "Message.h"
#include "OrderAction.h"

ONS_NAMESPACE_BEGIN

class MessageOrderListener {
public:
  virtual ~MessageOrderListener() = default;

  virtual OrderAction consume(const Message& message, const ConsumeOrderContext& context) noexcept = 0;
};

ONS_NAMESPACE_END