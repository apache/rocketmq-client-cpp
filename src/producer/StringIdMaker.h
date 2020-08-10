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
#ifndef __STRINGID_MAKER_H__
#define __STRINGID_MAKER_H__

#include <atomic>
#include <cstdint>
#include <string>

namespace rocketmq {

class StringIdMaker {
 private:
  StringIdMaker();
  ~StringIdMaker();

 public:
  static StringIdMaker& getInstance() {
    // After c++11, the initialization occurs exactly once
    static StringIdMaker singleton_;
    return singleton_;
  }

  /* ID format:
   *   ip: 4 bytes
   *   pid: 2 bytes
   *   random: 4 bytes
   *   time: 4 bytes
   *   auto num: 2 bytes
   */
  std::string createUniqID();

 private:
  void setStartTime(uint64_t millis);

  static uint32_t getIP();
  static void hexdump(unsigned char* buffer, char* out_buff, std::size_t index);

 private:
  uint64_t mStartTime;
  uint64_t mNextStartTime;
  std::atomic<uint16_t> mCounter;

  char kFixString[21];

  static const char sHexAlphabet[16];
};

}  // namespace rocketmq
#endif
