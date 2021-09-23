#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum AddressScheme : int8_t {
  IPv4 = 0,
  IPv6 = 1,
  DOMAIN_NAME = 2,
};

class Address {
public:
  Address(absl::string_view host, int32_t port) : host_(host.data(), host.length()), port_(port) {}

  bool operator==(const Address& other) const { return host_ == other.host_ && port_ == other.port_; }

  bool operator<(const Address& other) const {
    if (host_ < other.host_) {
      return true;
    } else if (host_ > other.host_) {
      return false;
    }

    if (port_ < other.port_) {
      return true;
    } else if (port_ > other.port_) {
      return false;
    }

    return false;
  }

  const std::string& host() const { return host_; }

  int32_t port() const { return port_; }

private:
  std::string host_;
  int32_t port_;
};

/**
 * Thread Safety Analysis:
 * Once initialized, this class remains immutable. Thus it is threads-safe.
 */
class ServiceAddress {
public:
  ServiceAddress(AddressScheme scheme, std::vector<Address> addresses)
      : scheme_(scheme), addresses_(std::move(addresses)) {
    std::sort(addresses_.begin(), addresses_.end());
  }

  AddressScheme scheme() const { return scheme_; }

  explicit operator bool() const { return !addresses_.empty(); }

  std::string address() const {
    std::string result;
    switch (scheme_) {
    case AddressScheme::IPv4:
      result.append("ipv4:");
      break;
    case AddressScheme::IPv6:
      result.append("ipv6:");
      break;
    case AddressScheme::DOMAIN_NAME:
      result.append("dns:");
      break;
    }

    bool first = true;
    for (const auto& item : addresses_) {
      if (!first) {
        result.append(",");
      } else {
        first = false;
      }
      result.append(item.host()).append(":").append(std::to_string(item.port()));
    }
    return result;
  }

private:
  AddressScheme scheme_;
  std::vector<Address> addresses_;
};

ROCKETMQ_NAMESPACE_END