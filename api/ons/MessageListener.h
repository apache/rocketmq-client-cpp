#pragma once

#include "Action.h"
#include "ConsumeContext.h"
#include "Message.h"

ONS_NAMESPACE_BEGIN
class ONSCLIENT_API MessageListener {
public:
  virtual ~MessageListener() = default;

  virtual Action consume(const Message& message, ConsumeContext& context) noexcept = 0;
};

ONS_NAMESPACE_END