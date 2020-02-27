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

#include <cstring>
#include <sstream>
#include <stdexcept>

#include <event2/event.h>

#ifndef WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "ByteOrder.h"
#include "MQClientException.h"

namespace rocketmq {

struct sockaddr IPPort2socketAddress(int host, int port) {
  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_port = htons((uint16_t)port);
  sin.sin_addr.s_addr = htonl(host);
  return *(struct sockaddr*)&sin;
}

std::string socketAddress2IPPort(const struct sockaddr* addr) {
  if (NULL == addr) {
    return std::string();
  }

  if (addr->sa_family == AF_INET) {
    struct sockaddr_in* sin = (struct sockaddr_in*)addr;
    std::ostringstream ipport;
    ipport << socketAddress2String(addr) << ":" << ntohs(sin->sin_port);
    return ipport.str();
  } else if (addr->sa_family == AF_INET6) {
    // struct sockaddr_in6* sin6 = (struct sockaddr_in6*)addr;
    throw std::runtime_error("don't support ipv6.");
  } else {
    throw std::runtime_error("don't support non-inet Address families.");
  }
}

void socketAddress2IPPort(const struct sockaddr* addr, int& host, int& port) {
  if (addr->sa_family == AF_INET) {
    struct sockaddr_in* sin = (struct sockaddr_in*)addr;
    host = ntohl(sin->sin_addr.s_addr);
    port = ntohs(sin->sin_port);
  } else if (addr->sa_family == AF_INET6) {
    // struct sockaddr_in6* sin6 = (struct sockaddr_in6*)addr;
    throw std::runtime_error("don't support ipv6.");
  } else {
    throw std::runtime_error("don't support non-inet Address families.");
  }
}

/**
 * converts an address from network format to presentation format (a.b.c.d)
 */
std::string socketAddress2String(const struct sockaddr* addr) {
  if (NULL == addr) {
    return std::string();
  }

  char buf[128];
  const char* address = NULL;

  if (addr->sa_family == AF_INET) {
    struct sockaddr_in* sin = (struct sockaddr_in*)addr;
    address = evutil_inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf));
  } else if (addr->sa_family == AF_INET6) {
    struct sockaddr_in6* sin6 = (struct sockaddr_in6*)addr;
    address = evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf));
  }

  if (address != NULL) {
    return address;
  }

  return std::string();
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

std::string lookupNameServers(const std::string& hostname) {
  if (hostname.empty()) {
    return "127.0.0.1";
  }

  std::string ip;

  struct evutil_addrinfo hints;
  struct evutil_addrinfo* answer = NULL;

  /* Build the hints to tell getaddrinfo how to act. */
  memset(&hints, 0, sizeof(hints));
  // hints.ai_family = AF_UNSPEC;           /* v4 or v6 is fine. */
  hints.ai_family = AF_INET;             /* v4 only. */
  hints.ai_socktype = SOCK_STREAM;       /* We want stream socket*/
  hints.ai_protocol = IPPROTO_TCP;       /* We want a TCP socket */
  hints.ai_flags = EVUTIL_AI_ADDRCONFIG; /* Only return addresses we can use. */

  // Look up the hostname.
  int err = evutil_getaddrinfo(hostname.c_str(), NULL, &hints, &answer);
  if (err != 0) {
    std::string info = "Failed to resolve host name(" + hostname + "): " + evutil_gai_strerror(err);
    THROW_MQEXCEPTION(UnknownHostException, info, -1);
  }

  for (struct evutil_addrinfo* addressInfo = answer; addressInfo != NULL; addressInfo = addressInfo->ai_next) {
    ip = socketAddress2String(addressInfo->ai_addr);
    if (!ip.empty()) {
      break;
    }
  }

  evutil_freeaddrinfo(answer);

  return ip;
}

uint64_t h2nll(uint64_t v) {
  return ByteOrder::swapIfLittleEndian(v);
}

uint64_t n2hll(uint64_t v) {
  return ByteOrder::swapIfLittleEndian(v);
}

}  // namespace rocketmq
