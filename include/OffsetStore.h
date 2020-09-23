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
#ifndef ROCKETMQ_OFFSETSTORE_H_
#define ROCKETMQ_OFFSETSTORE_H_

#include <vector>  // std::vector

#include "MQMessageQueue.h"

namespace rocketmq {

enum ReadOffsetType {
  // read offset from memory
  READ_FROM_MEMORY,
  // read offset from remoting
  READ_FROM_STORE,
  // read offset from memory firstly, then from remoting
  MEMORY_FIRST_THEN_STORE,
};

class ROCKETMQCLIENT_API OffsetStore {
 public:
  virtual ~OffsetStore() = default;

  virtual void load() = 0;
  virtual void updateOffset(const MQMessageQueue& mq, int64_t offset, bool increaseOnly) = 0;
  virtual int64_t readOffset(const MQMessageQueue& mq, ReadOffsetType type) = 0;
  virtual void persist(const MQMessageQueue& mq) = 0;
  virtual void persistAll(std::vector<MQMessageQueue>& mqs) = 0;
  virtual void removeOffset(const MQMessageQueue& mq) = 0;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_OFFSETSTORE_H_
