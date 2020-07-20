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
#include "MQMessageQueue.h"

#include <sstream>  // std::stringstream

namespace rocketmq {

MQMessageQueue::MQMessageQueue()
    : queue_id_(-1)  // invalide mq
{}

MQMessageQueue::MQMessageQueue(const std::string& topic, const std::string& brokerName, int queueId)
    : topic_(topic), broker_name_(brokerName), queue_id_(queueId) {}

MQMessageQueue::MQMessageQueue(const MQMessageQueue& other)
    : MQMessageQueue(other.topic_, other.broker_name_, other.queue_id_) {}

MQMessageQueue& MQMessageQueue::operator=(const MQMessageQueue& other) {
  if (this != &other) {
    broker_name_ = other.broker_name_;
    topic_ = other.topic_;
    queue_id_ = other.queue_id_;
  }
  return *this;
}

bool MQMessageQueue::operator==(const MQMessageQueue& other) const {
  return this == &other ||
         (queue_id_ == other.queue_id_ && broker_name_ == other.broker_name_ && topic_ == other.topic_);
}

bool MQMessageQueue::operator<(const MQMessageQueue& mq) const {
  return compareTo(mq) < 0;
}

int MQMessageQueue::compareTo(const MQMessageQueue& mq) const {
  int result = topic_.compare(mq.topic_);
  if (result != 0) {
    return result;
  }

  result = broker_name_.compare(mq.broker_name_);
  if (result != 0) {
    return result;
  }

  return queue_id_ - mq.queue_id_;
}

std::string MQMessageQueue::toString() const {
  std::stringstream ss;
  ss << "MessageQueue [topic=" << topic_ << ", brokerName=" << broker_name_ << ", queueId=" << queue_id_ << "]";
  return ss.str();
}

}  // namespace rocketmq
