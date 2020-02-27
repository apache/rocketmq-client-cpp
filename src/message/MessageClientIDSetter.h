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
#ifndef __MESSAGE_CLIENT_ID_SETTER_H__
#define __MESSAGE_CLIENT_ID_SETTER_H__

#include <atomic>
#include <cstdint>
#include <string>

#include "MQMessage.h"
#include "MessageAccessor.h"

namespace rocketmq {

class MessageClientIDSetter {
 public:
  static MessageClientIDSetter& getInstance() {
    // After c++11, the initialization occurs exactly once
    static MessageClientIDSetter singleton_;
    return singleton_;
  }

  /* ID format:
   *   ip: 4 bytes
   *   pid: 2 bytes
   *   random: 4 bytes
   *   time: 4 bytes
   *   auto num: 2 bytes
   */
  static std::string createUniqID() { return getInstance().createUniqueID(); }

  static void setUniqID(MQMessage& msg) {
    if (msg.getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX).empty()) {
      MessageAccessor::putProperty(msg, MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX, createUniqID());
    }
  }

  static const std::string& getUniqID(const MQMessage& msg) {
    return msg.getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
  }

 public:
  virtual ~MessageClientIDSetter();

 private:
  MessageClientIDSetter();

  void setStartTime(uint64_t millis);
  std::string createUniqueID();

 private:
  uint64_t mStartTime;
  uint64_t mNextStartTime;
  std::atomic<uint16_t> mCounter;

  std::string kFixString;
};

}  // namespace rocketmq

#endif  // __MESSAGE_CLIENT_ID_SETTER_H__
