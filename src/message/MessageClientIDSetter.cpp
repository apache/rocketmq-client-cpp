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

#include "ByteOrder.h"
#include "UtilAll.h"

namespace rocketmq {

MessageClientIDSetter::MessageClientIDSetter() {
  std::srand((uint32_t)std::time(NULL));

  uint32_t pid = ByteOrderUtil::NorminalBigEndian(static_cast<uint32_t>(UtilAll::getProcessId()));
  uint32_t ip = ByteOrderUtil::NorminalBigEndian(UtilAll::getIP());
  uint32_t random_num = ByteOrderUtil::NorminalBigEndian(static_cast<uint32_t>(std::rand()));

  char bin_buf[10];
  std::memcpy(bin_buf + 2, &pid, 4);
  std::memcpy(bin_buf, &ip, 4);
  std::memcpy(bin_buf + 6, &random_num, 4);

  kFixString = UtilAll::bytes2string(bin_buf, 10);

  setStartTime(UtilAll::currentTimeMillis());

  mCounter = 0;
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

  mStartTime = std::mktime(&curMonthBegin) * 1000;
  mNextStartTime = std::mktime(&nextMonthBegin) * 1000;
}

std::string MessageClientIDSetter::createUniqueID() {
  uint64_t current = UtilAll::currentTimeMillis();
  if (current >= mNextStartTime) {
    setStartTime(current);
    current = UtilAll::currentTimeMillis();
  }

  uint32_t period = ByteOrderUtil::NorminalBigEndian(static_cast<uint32_t>(current - mStartTime));
  uint16_t seqid = ByteOrderUtil::NorminalBigEndian(mCounter++);

  char bin_buf[6];
  std::memcpy(bin_buf, &period, 4);
  std::memcpy(bin_buf + 4, &seqid, 2);

  return kFixString + UtilAll::bytes2string(bin_buf, 6);
}

}  // namespace rocketmq
