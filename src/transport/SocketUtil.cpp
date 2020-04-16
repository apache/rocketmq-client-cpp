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

#include <event2/event.h>

#include <cstring>
#include <sstream>
#include <stdexcept>

#ifndef WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "ByteOrder.h"
#include "MQClientException.h"
#include "UtilAll.h"

namespace rocketmq {

struct sockaddr IPPort2socketAddress(int host, int port) {
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons((uint16_t)port);
  sin.sin_addr.s_addr = htonl(host);
  return *(struct sockaddr*)&sin;
}

const struct sockaddr* string2SocketAddress(const std::string& addr) {
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

/**
 * get the hostname of address
 */
std::string getHostName(const struct sockaddr* addr) {
  if (NULL == addr) {
    return std::string();
  }

  struct hostent* host = NULL;
  if (addr->sa_family == AF_INET) {
    struct sockaddr_in* sin = (struct sockaddr_in*)addr;
    host = ::gethostbyaddr((char*)&sin->sin_addr, sizeof(sin->sin_addr), AF_INET);
  } else if (addr->sa_family == AF_INET6) {
    struct sockaddr_in6* sin6 = (struct sockaddr_in6*)addr;
    host = ::gethostbyaddr((char*)&sin6->sin6_addr, sizeof(sin6->sin6_addr), AF_INET6);
  }

  if (host != NULL) {
    char** alias = host->h_aliases;
    if (*alias != NULL) {
      return *alias;
    }
  }

  return socketAddress2String(addr);
}

const struct sockaddr* lookupNameServers(const std::string& hostname) {
  if (hostname.empty()) {
    return nullptr;
  }

  struct evutil_addrinfo hints;
  struct evutil_addrinfo* answer = NULL;

  /* Build the hints to tell getaddrinfo how to act. */
  memset(&hints, 0, sizeof(hints));
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

  thread_local static union {
    struct sockaddr_in sin;
    struct sockaddr_in6 sin6;
  } sin_buf;
  struct sockaddr* sin = nullptr;

  for (struct evutil_addrinfo* ai = answer; ai != NULL; ai = ai->ai_next) {
    auto* ai_addr = ai->ai_addr;
    if (ai_addr->sa_family != AF_INET && ai_addr->sa_family != AF_INET6) {
      continue;
    }
    sin = (struct sockaddr*)&sin_buf;
    memcpy(sin, ai_addr, ai_addr->sa_len);
    break;
  }

  evutil_freeaddrinfo(answer);

  return sin;
}

uint64_t h2nll(uint64_t v) {
  return ByteOrder::swapIfLittleEndian(v);
}

uint64_t n2hll(uint64_t v) {
  return ByteOrder::swapIfLittleEndian(v);
}

}  // namespace rocketmq
