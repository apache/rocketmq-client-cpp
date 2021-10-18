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
#pragma once

#include <map>
#include <string>

#include "ConsumeMessageType.h"
#include "rocketmq/MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

class Assignment {
public:
  Assignment(MQMessageQueue message_queue) : message_queue_(std::move(message_queue)) {
  }

  bool operator==(const Assignment& rhs) const {
    if (this == &rhs) {
      return true;
    }

    return message_queue_ == rhs.message_queue_;
  }

  bool operator<(const Assignment& rhs) const {
    return message_queue_ < rhs.message_queue_;
  }

  const MQMessageQueue& messageQueue() const {
    return message_queue_;
  }

private:
  MQMessageQueue message_queue_;
};
ROCKETMQ_NAMESPACE_END