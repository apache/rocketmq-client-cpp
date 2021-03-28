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
#ifndef ROCKETMQ_TRANSPORT_SOCKETUTIL_H_
#define ROCKETMQ_TRANSPORT_SOCKETUTIL_H_

#include <cassert>  // assert
#include <cstddef>  // size_t
#include <cstdint>  // uint16_t

#include <string>

#ifndef WIN32
#include <netinet/in.h>  // sockaddr_in, AF_INET, sockaddr_in6, AF_INET6
#include <sys/socket.h>  // sockaddr, sockaddr_storage
#else
#include <Winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "ByteArray.h"

namespace rocketmq {

const size_t kIPv4AddrSize = 4;
const size_t kIPv6AddrSize = 16;

static inline size_t IpaddrSize(const sockaddr* sa) {
  assert(sa != nullptr);
  assert(sa->sa_family == AF_INET || sa->sa_family == AF_INET6);
  return sa->sa_family == AF_INET6 ? kIPv6AddrSize : kIPv4AddrSize;
}

static inline size_t IpaddrSize(const sockaddr_storage* ss) {
  return IpaddrSize(reinterpret_cast<const sockaddr*>(ss));
}

static inline size_t SockaddrSize(const sockaddr* sa) {
  assert(sa != nullptr);
  assert(sa->sa_family == AF_INET || sa->sa_family == AF_INET6);
  return sa->sa_family == AF_INET6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in);
}

static inline size_t SockaddrSize(const sockaddr_storage* ss) {
  return SockaddrSize(reinterpret_cast<const sockaddr*>(ss));
}

std::unique_ptr<sockaddr_storage> SockaddrToStorage(const sockaddr* src);

sockaddr* IPPortToSockaddr(const ByteArray& ip, uint16_t port);

sockaddr* StringToSockaddr(const std::string& addr);
std::string SockaddrToString(const sockaddr* addr);

sockaddr* LookupNameServers(const std::string& hostname);

sockaddr* GetSelfIP();
const std::string& GetLocalHostname();
const std::string& GetLocalAddress();

}  // namespace rocketmq

#endif  // ROCKETMQ_TRANSPORT_SOCKETUTIL_H_
