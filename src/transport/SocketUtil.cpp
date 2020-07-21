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

#include <cstdlib>  // std::realloc
#include <cstring>  // std::memset, std::memcpy

#include <sstream>
#include <stdexcept>

#include <event2/event.h>

#ifndef WIN32
#include <netdb.h>
#include <unistd.h>
#endif

#include "ByteOrder.h"
#include "MQException.h"
#include "UtilAll.h"

namespace rocketmq {

union sockaddr_union {
  struct sockaddr_in sin;
  struct sockaddr_in6 sin6;
};

thread_local static sockaddr_union sin_buf;

struct sockaddr* ipPort2SocketAddress(const ByteArray& ip, uint16_t port) {
  if (ip.size() == 4) {
    struct sockaddr_in* sin = &sin_buf.sin;
    sin->sin_family = AF_INET;
    sin->sin_port = ByteOrderUtil::NorminalBigEndian<uint16_t>(port);
    ByteOrderUtil::Read<decltype(sin->sin_addr)>(&sin->sin_addr, ip.array());
    return (struct sockaddr*)sin;
  } else if (ip.size() == 16) {
    struct sockaddr_in6* sin6 = &sin_buf.sin6;
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = ByteOrderUtil::NorminalBigEndian<uint16_t>(port);
    ByteOrderUtil::Read<decltype(sin6->sin6_addr)>(&sin6->sin6_addr, ip.array());
    return (struct sockaddr*)sin6;
  }
  return nullptr;
}

struct sockaddr* string2SocketAddress(const std::string& addr) {
  std::string::size_type start_pos = addr[0] == '/' ? 1 : 0;
  auto colon_pos = addr.find_last_of(":");
  std::string host = addr.substr(start_pos, colon_pos - start_pos);
  std::string port = addr.substr(colon_pos + 1, addr.length() - colon_pos);
  auto* sa = lookupNameServers(host);
  if (sa != nullptr) {
    if (sa->sa_family == AF_INET) {
      auto* sin = (struct sockaddr_in*)sa;
      sin->sin_port = htons((uint16_t)std::stoi(port));
    } else {
      auto* sin6 = (struct sockaddr_in6*)sa;
      sin6->sin6_port = htons((uint16_t)std::stoi(port));
    }
  }
  return sa;
}

/**
 * converts an address from network format to presentation format (a.b.c.d)
 */
std::string socketAddress2String(const struct sockaddr* addr) {
  if (nullptr == addr) {
    return "127.0.0.1";
  }

  char buf[128];
  std::string address;
  uint16_t port = 0;

  if (addr->sa_family == AF_INET) {
    auto* sin = (struct sockaddr_in*)addr;
    if (nullptr != evutil_inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf))) {
      address = buf;
    }
    port = ntohs(sin->sin_port);
  } else if (addr->sa_family == AF_INET6) {
    auto* sin6 = (struct sockaddr_in6*)addr;
    if (nullptr != evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf))) {
      address = buf;
    }
    port = ntohs(sin6->sin6_port);
  } else {
    throw std::runtime_error("don't support non-inet Address families.");
  }

  if (!address.empty() && port != 0) {
    if (addr->sa_family == AF_INET6) {
      address = "[" + address + "]";
    }
    address += ":" + UtilAll::to_string(port);
  }

  return address;
}

struct sockaddr* lookupNameServers(const std::string& hostname) {
  if (hostname.empty()) {
    return nullptr;
  }

  struct evutil_addrinfo hints;
  struct evutil_addrinfo* answer = NULL;

  /* Build the hints to tell getaddrinfo how to act. */
  std::memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;           /* v4 or v6 is fine. */
  hints.ai_socktype = SOCK_STREAM;       /* We want stream socket*/
  hints.ai_protocol = IPPROTO_TCP;       /* We want a TCP socket */
  hints.ai_flags = EVUTIL_AI_ADDRCONFIG; /* Only return addresses we can use. */

  // Look up the hostname.
  int err = evutil_getaddrinfo(hostname.c_str(), NULL, &hints, &answer);
  if (err != 0) {
    std::string info = "Failed to resolve host name(" + hostname + "): " + evutil_gai_strerror(err);
    THROW_MQEXCEPTION(UnknownHostException, info, -1);
  }

  struct sockaddr* sin = nullptr;

  for (struct evutil_addrinfo* ai = answer; ai != NULL; ai = ai->ai_next) {
    auto* ai_addr = ai->ai_addr;
    if (ai_addr->sa_family != AF_INET && ai_addr->sa_family != AF_INET6) {
      continue;
    }
    sin = (struct sockaddr*)&sin_buf;
    std::memcpy(sin, ai_addr, sockaddr_size(ai_addr));
    break;
  }

  evutil_freeaddrinfo(answer);

  return sin;
}

struct sockaddr* copySocketAddress(struct sockaddr* dst, const struct sockaddr* src) {
  if (src != nullptr) {
    if (dst == nullptr || dst->sa_family != src->sa_family) {
      dst = (struct sockaddr*)std::realloc(dst, sizeof(union sockaddr_union));
    }
    std::memcpy(dst, src, sockaddr_size(src));
  } else {
    free(dst);
    dst = nullptr;
  }
  return dst;
}

uint64_t h2nll(uint64_t v) {
  return ByteOrderUtil::NorminalBigEndian(v);
}

uint64_t n2hll(uint64_t v) {
  return ByteOrderUtil::NorminalBigEndian(v);
}

}  // namespace rocketmq
