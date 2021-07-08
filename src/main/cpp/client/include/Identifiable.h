#pragma once

#include "rocketmq/RocketMQ.h"
#include "rocketmq/CredentialsProvider.h"
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

class Identifiable {
public:
  virtual ~Identifiable() = default;

  virtual CredentialsProviderPtr credentialsProvider() = 0;

  virtual const std::string& tenantId() const = 0;
};

using IdentifiablePtr = std::shared_ptr<Identifiable>;

ROCKETMQ_NAMESPACE_END