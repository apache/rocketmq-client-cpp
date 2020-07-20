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
#ifndef __SOCKET_UTIL_H__
#define __SOCKET_UTIL_H__

#include <cstdint>

#include <string>

#ifndef WIN32
#include <arpa/inet.h>
#include <sys/socket.h>
#else
#include <Winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "ByteArray.h"

namespace rocketmq {

static inline size_t sockaddr_size(const struct sockaddr* sa) {
  return sa->sa_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
}

struct sockaddr* ipPort2SocketAddress(const ByteArray& ip, uint16_t port);

struct sockaddr* string2SocketAddress(const std::string& addr);
std::string socketAddress2String(const struct sockaddr* addr);

struct sockaddr* lookupNameServers(const std::string& hostname);

struct sockaddr* copySocketAddress(struct sockaddr* dst, const struct sockaddr* src);

uint64_t h2nll(uint64_t v);
uint64_t n2hll(uint64_t v);

}  // namespace rocketmq

#endif  // __SOCKET_UTIL_H__
