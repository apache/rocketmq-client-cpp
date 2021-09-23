#pragma once

#include <memory>
#include <vector>

#include "ServiceAddress.h"

ROCKETMQ_NAMESPACE_BEGIN

class Broker {
public:
  Broker(std::string name, int id, ServiceAddress service_address)
      : name_(std::move(name)), id_(id), service_address_(std::move(service_address)) {}

  const std::string& name() const { return name_; }

  int32_t id() const { return id_; }

  explicit operator bool() const { return service_address_.operator bool(); }

  bool operator==(const Broker& other) const { return name_ == other.name_; }

  bool operator<(const Broker& other) const { return name_ < other.name_; }

  std::string serviceAddress() const { return service_address_.address(); }

private:
  std::string name_;
  int32_t id_;
  ServiceAddress service_address_;
};

ROCKETMQ_NAMESPACE_END