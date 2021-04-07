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
#include "SocketUtil.h"

#include <cstdlib>  // std::abort
#include <cstring>  // std::memcpy, std::memset

#include <iostream>
#include <limits>
#include <stdexcept>  // std::invalid_argument, std::runtime_error
#include <string>

#ifndef WIN32
#include <arpa/inet.h>  // htons
#include <unistd.h>     // gethostname
#else
#include <Winsock2.h>
#endif

#include <event2/event.h>

#include "MQException.h"

namespace rocketmq {

std::unique_ptr<sockaddr_storage> SockaddrToStorage(const sockaddr* src) {
  if (src == nullptr) {
    return nullptr;
  }
  std::unique_ptr<sockaddr_storage> ss(new sockaddr_storage);
  std::memcpy(ss.get(), src, SockaddrSize(src));
  return ss;
}

thread_local sockaddr_storage ss_buffer;

sockaddr* IPPortToSockaddr(const ByteArray& ip, uint16_t port) {
  sockaddr_storage* ss = &ss_buffer;
  if (ip.size() == kIPv4AddrSize) {
    auto* sin = reinterpret_cast<sockaddr_in*>(ss);
    sin->sin_family = AF_INET;
    sin->sin_port = htons(port);
    std::memcpy(&sin->sin_addr, ip.array(), kIPv4AddrSize);
  } else if (ip.size() == kIPv6AddrSize) {
    auto* sin6 = reinterpret_cast<sockaddr_in6*>(ss);
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = htons(port);
    std::memcpy(&sin6->sin6_addr, ip.array(), kIPv6AddrSize);
  } else {
    throw std::invalid_argument("invalid ip size");
  }
  return reinterpret_cast<sockaddr*>(ss);
}

sockaddr* StringToSockaddr(const std::string& addr) {
  if (addr.empty()) {
    throw std::invalid_argument("invalid address");
  }

  std::string::size_type start_pos = addr[0] == '/' ? 1 : 0;
  auto colon_pos = addr.find_last_of(':');
  auto bracket_pos = addr.find_last_of(']');
  if (bracket_pos != std::string::npos) {
    // ipv6 address
    if (addr.at(start_pos) != '[') {
      throw std::invalid_argument("invalid address");
    }
    if (colon_pos == std::string::npos) {
      throw std::invalid_argument("invalid address");
    }
    if (colon_pos < bracket_pos) {
      // have not port
      if (bracket_pos != addr.size() - 1) {
        throw std::invalid_argument("invalid address");
      }
      colon_pos = addr.size();
    } else if (colon_pos != bracket_pos + 1) {
      throw std::invalid_argument("invalid address");
    }
  } else if (colon_pos == std::string::npos) {
    // have not port
    colon_pos = addr.size();
  }

  decltype(bracket_pos) fix_bracket = bracket_pos == std::string::npos ? 0 : 1;
  std::string host = addr.substr(start_pos + fix_bracket, colon_pos - start_pos - fix_bracket * 2);
  auto* sa = LookupNameServers(host);

  std::string port = colon_pos >= addr.size() ? "0" : addr.substr(colon_pos + 1);
  uint32_t n = std::stoul(port);
  if (n > std::numeric_limits<uint16_t>::max()) {
    throw std::out_of_range("port is to large");
  }
  uint16_t port_num = htons(static_cast<uint16_t>(n));

  if (sa->sa_family == AF_INET) {
    auto* sin = reinterpret_cast<sockaddr_in*>(sa);
    sin->sin_port = port_num;
  } else if (sa->sa_family == AF_INET6) {
    auto* sin6 = reinterpret_cast<sockaddr_in6*>(sa);
    sin6->sin6_port = port_num;
  } else {
    throw std::runtime_error("don't support non-inet address families");
  }

  return sa;
}

/**
 * converts an address from network format to presentation format
 */
std::string SockaddrToString(const sockaddr* addr) {
  if (nullptr == addr) {
    return std::string();
  }

  char buf[128];
  std::string address;
  uint16_t port = 0;

  if (addr->sa_family == AF_INET) {
    const auto* sin = reinterpret_cast<const sockaddr_in*>(addr);
    if (nullptr == evutil_inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
      throw std::runtime_error("can not convert AF_INET address to text form");
    }
    address = buf;
    port = ntohs(sin->sin_port);
  } else if (addr->sa_family == AF_INET6) {
    const auto* sin6 = reinterpret_cast<const sockaddr_in6*>(addr);
    if (nullptr == evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf))) {
      throw std::runtime_error("can not convert AF_INET6 address to text form");
    }
    address = buf;
    port = ntohs(sin6->sin6_port);
  } else {
    throw std::runtime_error("don't support non-inet address families");
  }

  if (addr->sa_family == AF_INET6) {
    address = "[" + address + "]";
  }
  if (port != 0) {
    address += ":" + std::to_string(port);
  }

  return address;
}

sockaddr* LookupNameServers(const std::string& hostname) {
  if (hostname.empty()) {
    throw std::invalid_argument("invalid hostname");
  }

  evutil_addrinfo hints;
  evutil_addrinfo* answer = nullptr;

  /* Build the hints to tell getaddrinfo how to act. */
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;           /* v4 or v6 is fine. */
  hints.ai_socktype = SOCK_STREAM;       /* We want stream socket*/
  hints.ai_protocol = IPPROTO_TCP;       /* We want a TCP socket */
  hints.ai_flags = EVUTIL_AI_ADDRCONFIG; /* Only return addresses we can use. */

  // Look up the hostname.
  int err = evutil_getaddrinfo(hostname.c_str(), nullptr, &hints, &answer);
  if (err != 0) {
    std::string info = "Failed to resolve hostname(" + hostname + "): " + evutil_gai_strerror(err);
    THROW_MQEXCEPTION(UnknownHostException, info, -1);
  }

  sockaddr_storage* ss = &ss_buffer;

  bool hit = false;
  for (struct evutil_addrinfo* ai = answer; ai != nullptr; ai = ai->ai_next) {
    auto* ai_addr = ai->ai_addr;
    if (ai_addr->sa_family != AF_INET && ai_addr->sa_family != AF_INET6) {
      continue;
    }
    std::memcpy(ss, ai_addr, SockaddrSize(ai_addr));
    hit = true;
    break;
  }

  evutil_freeaddrinfo(answer);

  if (!hit) {
    throw std::runtime_error("hostname is non-inet address family");
  }

  return reinterpret_cast<sockaddr*>(ss);
}

sockaddr* GetSelfIP() {
  try {
    return LookupNameServers(GetLocalHostname());
  } catch (const UnknownHostException& e) {
    return LookupNameServers("localhost");
  }
}

const std::string& GetLocalHostname() {
  static std::string local_hostname = []() {
    char name[1024];
    if (::gethostname(name, sizeof(name)) != 0) {
      return std::string();
    }
    return std::string(name);
  }();
  return local_hostname;
}

const std::string& GetLocalAddress() {
  static std::string local_address = []() {
    try {
      return SockaddrToString(GetSelfIP());
    } catch (std::exception& e) {
      std::cerr << e.what() << std::endl;
      std::abort();
    }
  }();
  return local_address;
}

}  // namespace rocketmq
