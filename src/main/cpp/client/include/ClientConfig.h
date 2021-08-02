#pragma once

#include "rocketmq/CredentialsProvider.h"
#include "absl/time/time.h"
#include "rocketmq/RocketMQ.h"
#include <string>

ROCKETMQ_NAMESPACE_BEGIN

class ClientConfig {
public:
  virtual ~ClientConfig() = default;

  virtual const std::string& region() const = 0;

  virtual const std::string& serviceName() const = 0;

  virtual const std::string& arn() const = 0;

  virtual CredentialsProviderPtr credentialsProvider() = 0;

  virtual const std::string& tenantId() const = 0;

  virtual absl::Duration getIoTimeout() const = 0;

  virtual absl::Duration getLongPollingTimeout() const = 0;

  virtual const std::string& getGroupName() const = 0;

  virtual std::string clientId() const = 0;
};

ROCKETMQ_NAMESPACE_END