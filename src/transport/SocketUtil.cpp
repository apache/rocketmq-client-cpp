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

#include "ByteOrder.h"
#include "MQClientException.h"

namespace rocketmq {
//<!***************************************************************************
sockaddr IPPort2socketAddress(int host, int port) {
  struct sockaddr_in sa;
  sa.sin_family = AF_INET;
  sa.sin_port = htons((uint16)port);
  sa.sin_addr.s_addr = htonl(host);

  sockaddr bornAddr;
  memcpy(&bornAddr, &sa, sizeof(sockaddr));
  return bornAddr;
}

std::string socketAddress2IPPort(sockaddr addr) {
  sockaddr_in sa;
  memcpy(&sa, &addr, sizeof(sockaddr));

  char tmp[32];
  sprintf(tmp, "%s:%d", inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));

  std::string ipport = tmp;
  return ipport;
}

void socketAddress2IPPort(sockaddr addr, int& host, int& port) {
  struct sockaddr_in sa;
  memcpy(&sa, &addr, sizeof(sockaddr));

  host = ntohl(sa.sin_addr.s_addr);
  port = ntohs(sa.sin_port);
}

std::string socketAddress2String(sockaddr addr) {
  sockaddr_in in;
  memcpy(&in, &addr, sizeof(sockaddr));

  return inet_ntoa(in.sin_addr);
}

std::string getHostName(sockaddr addr) {
  sockaddr_in in;
  memcpy(&in, &addr, sizeof(sockaddr));

  struct hostent* remoteHost = gethostbyaddr((char*)&(in.sin_addr), 4, AF_INET);
  char** alias = remoteHost->h_aliases;
  if (*alias != 0) {
    return *alias;
  } else {
    return inet_ntoa(in.sin_addr);
  }
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
    THROW_MQEXCEPTION(MQClientException, info, -1);
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
