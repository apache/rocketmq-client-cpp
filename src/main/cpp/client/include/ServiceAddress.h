/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "absl/strings/string_view.h"

#include "rocketmq/RocketMQ.h"

ROCKETMQ_NAMESPACE_BEGIN

enum AddressScheme : int8_t
{
  IPv4 = 0,
  IPv6 = 1,
  DOMAIN_NAME = 2,
};

class Address {
public:
  Address(absl::string_view host, int32_t port) : host_(host.data(), host.length()), port_(port) {
  }

  bool operator==(const Address& other) const {
    return host_ == other.host_ && port_ == other.port_;
  }

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

  const std::string& host() const {
    return host_;
  }

  int32_t port() const {
    return port_;
  }

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

  AddressScheme scheme() const {
    return scheme_;
  }

  explicit operator bool() const {
    return !addresses_.empty();
  }

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