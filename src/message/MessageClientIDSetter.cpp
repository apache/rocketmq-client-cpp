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
#include "MessageClientIDSetter.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#ifndef WIN32
#include <unistd.h>
#endif

#include "ByteBuffer.hpp"
#include "SocketUtil.h"
#include "UtilAll.h"

namespace rocketmq {

MessageClientIDSetter::MessageClientIDSetter() {
  std::srand((uint32_t)std::time(NULL));

  std::unique_ptr<ByteBuffer> buffer;
  sockaddr* addr = GetSelfIP();
  if (addr != nullptr) {
    buffer.reset(ByteBuffer::allocate(SockaddrSize(addr) + 2 + 4));
    if (addr->sa_family == AF_INET) {
      auto* sin = (struct sockaddr_in*)addr;
      buffer->put(ByteArray(reinterpret_cast<char*>(&sin->sin_addr), kIPv4AddrSize));
    } else if (addr->sa_family == AF_INET6) {
      auto* sin6 = (struct sockaddr_in6*)addr;
      buffer->put(ByteArray(reinterpret_cast<char*>(&sin6->sin6_addr), kIPv6AddrSize));
    } else {
      (void)buffer.release();
    }
  }
  if (buffer == nullptr) {
    buffer.reset(ByteBuffer::allocate(4 + 2 + 4));
    buffer->putInt(UtilAll::currentTimeMillis());
  }
  buffer->putShort(UtilAll::getProcessId());
  buffer->putInt(std::rand());

  fixed_string_ = UtilAll::bytes2string(buffer->array(), buffer->position());

  setStartTime(UtilAll::currentTimeMillis());

  counter_ = 0;
}

MessageClientIDSetter::~MessageClientIDSetter() = default;

void MessageClientIDSetter::setStartTime(uint64_t millis) {
  // std::time_t
  //   Although not defined, this is almost always an integral value holding the number of seconds
  //   (not counting leap seconds) since 00:00, Jan 1 1970 UTC, corresponding to POSIX time.
  std::time_t tmNow = millis / 1000;
  std::tm* ptmNow = std::localtime(&tmNow);  // may not be thread-safe

  std::tm curMonthBegin = {0};
  curMonthBegin.tm_year = ptmNow->tm_year;  // since 1900
  curMonthBegin.tm_mon = ptmNow->tm_mon;    // [0, 11]
  curMonthBegin.tm_mday = 1;                // [1, 31]
  curMonthBegin.tm_hour = 0;                // [0, 23]
  curMonthBegin.tm_min = 0;                 // [0, 59]
  curMonthBegin.tm_sec = 0;                 // [0, 60]

  std::tm nextMonthBegin = {0};
  if (ptmNow->tm_mon >= 11) {
    nextMonthBegin.tm_year = ptmNow->tm_year + 1;
    nextMonthBegin.tm_mon = 0;
  } else {
    nextMonthBegin.tm_year = ptmNow->tm_year;
    nextMonthBegin.tm_mon = ptmNow->tm_mon + 1;
  }
  nextMonthBegin.tm_mday = 1;
  nextMonthBegin.tm_hour = 0;
  nextMonthBegin.tm_min = 0;
  nextMonthBegin.tm_sec = 0;

  start_time_ = std::mktime(&curMonthBegin) * 1000;
  next_start_time_ = std::mktime(&nextMonthBegin) * 1000;
}

std::string MessageClientIDSetter::createUniqueID() {
  uint64_t current = UtilAll::currentTimeMillis();
  if (current >= next_start_time_) {
    setStartTime(current);
    current = UtilAll::currentTimeMillis();
  }

  uint32_t period = ByteOrderUtil::NorminalBigEndian(static_cast<uint32_t>(current - start_time_));
  uint16_t seqid = ByteOrderUtil::NorminalBigEndian(counter_++);

  return fixed_string_ + UtilAll::bytes2string(reinterpret_cast<char*>(&period), sizeof(period)) +
         UtilAll::bytes2string(reinterpret_cast<char*>(&seqid), sizeof(seqid));
}

}  // namespace rocketmq
