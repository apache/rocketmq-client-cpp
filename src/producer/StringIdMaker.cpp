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
#include "StringIdMaker.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "ByteOrder.h"
#include "UtilAll.h"

namespace rocketmq {

const char StringIdMaker::sHexAlphabet[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                              '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

StringIdMaker::StringIdMaker() {
  std::srand((uint32_t)std::time(NULL));

  uint32_t pid = ByteOrder::swapIfLittleEndian(static_cast<uint32_t>(getpid()));
  uint32_t ip = ByteOrder::swapIfLittleEndian(getIP());
  uint32_t random_num = ByteOrder::swapIfLittleEndian(static_cast<uint32_t>(std::rand()));

  unsigned char bin_buf[10];
  std::memcpy(bin_buf + 2, &pid, 4);
  std::memcpy(bin_buf, &ip, 4);
  std::memcpy(bin_buf + 6, &random_num, 4);

  hexdump(bin_buf, kFixString, 10);
  kFixString[20] = '\0';

  setStartTime(UtilAll::currentTimeMillis());

  mCounter = 0;
}

StringIdMaker::~StringIdMaker() {}

uint32_t StringIdMaker::getIP() {
  std::string ip = UtilAll::getLocalAddress();
  if (ip.empty()) {
    return 0;
  }

  char* ip_str = new char[ip.length() + 1];
  std::strncpy(ip_str, ip.c_str(), ip.length());
  ip_str[ip.length()] = '\0';

  int i = 3;
  uint32_t nResult = 0;
  for (char* token = std::strtok(ip_str, "."); token != nullptr && i >= 0; token = std::strtok(nullptr, ".")) {
    uint32_t n = std::atoi(token);
    nResult |= n << (8 * i--);
  }

  delete[] ip_str;

  return nResult;
}

void StringIdMaker::setStartTime(uint64_t millis) {
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

  mStartTime = std::mktime(&curMonthBegin) * 1000;
  mNextStartTime = std::mktime(&nextMonthBegin) * 1000;
}

std::string StringIdMaker::createUniqID() {
  uint64_t current = UtilAll::currentTimeMillis();
  if (current >= mNextStartTime) {
    setStartTime(current);
    current = UtilAll::currentTimeMillis();
  }

  uint32_t period = ByteOrder::swapIfLittleEndian(static_cast<uint32_t>(current - mStartTime));
  uint16_t seqid = ByteOrder::swapIfLittleEndian(mCounter++);

  unsigned char bin_buf[6];
  std::memcpy(bin_buf, &period, 4);
  std::memcpy(bin_buf + 4, &seqid, 2);

  char hex_buf[12];
  hexdump(bin_buf, hex_buf, 6);

  return std::string(kFixString, 20) + std::string(hex_buf, 12);
}

void StringIdMaker::hexdump(unsigned char* buffer, char* out_buff, std::size_t index) {
  for (std::size_t i = 0; i < index; i++) {
    unsigned char v = buffer[i];
    out_buff[i * 2] = sHexAlphabet[v >> 4];
    out_buff[i * 2 + 1] = sHexAlphabet[v & 0x0FU];
  }
}

}  // namespace rocketmq
