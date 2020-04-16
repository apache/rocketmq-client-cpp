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
#ifndef __MESSAGE_ID_H__
#define __MESSAGE_ID_H__

#include "SocketUtil.h"
#include "UtilAll.h"

namespace rocketmq {

class MQMessageId {
 public:
  MQMessageId() : MQMessageId(nullptr, 0) {}
  MQMessageId(struct sockaddr* address, int64_t offset) : m_address(nullptr), m_offset(offset) { setAddress(address); }
  ~MQMessageId() { free(m_address); }

  MQMessageId& operator=(const MQMessageId& id) {
    if (&id == this) {
      return *this;
    }
    setAddress(id.m_address);
    this->m_offset = id.m_offset;
    return *this;
  }

  const struct sockaddr* getAddress() const { return m_address; }

  void setAddress(struct sockaddr* address) { m_address = copySocketAddress(m_address, address); }

  int64_t getOffset() const { return m_offset; }

  void setOffset(int64_t offset) { m_offset = offset; }

 private:
  struct sockaddr* m_address;
  int64_t m_offset;
};

}  // namespace rocketmq

#endif  // __MESSAGE_ID_H__
