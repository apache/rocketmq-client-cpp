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

#include "Broker.h"
#include "Topic.h"
#include "rocketmq/MQMessage.h"
#include "rocketmq/MQMessageQueue.h"

ROCKETMQ_NAMESPACE_BEGIN

enum Permission : int8_t
{
  NONE = 0,
  READ = 1,
  WRITE = 2,
  READ_WRITE = 3
};

class Partition {
public:
  Partition(Topic topic, int32_t id, Permission permission, Broker broker)
      : topic_(std::move(topic)), broker_(std::move(broker)), id_(id), permission_(permission) {
  }

  const Topic& topic() const {
    return topic_;
  }

  int32_t id() const {
    return id_;
  }

  const Broker& broker() const {
    return broker_;
  }

  Permission permission() const {
    return permission_;
  }

  MQMessageQueue asMessageQueue() const {
    MQMessageQueue message_queue(topic_.name(), broker_.name(), id_);
    message_queue.serviceAddress(broker_.serviceAddress());
    return message_queue;
  }

  bool operator==(const Partition& other) const {
    return topic_ == other.topic_ && id_ == other.id_ && broker_ == other.broker_ && permission_ == other.permission_;
  }

  bool operator<(const Partition& other) const {
    if (topic_ < other.topic_) {
      return true;
    } else if (other.topic_ < topic_) {
      return false;
    }

    if (broker_ < other.broker_) {
      return true;
    } else if (other.broker_ < broker_) {
      return false;
    }

    return id_ < other.id_;
  }

private:
  Topic topic_;
  Broker broker_;
  int32_t id_;
  Permission permission_;
};

ROCKETMQ_NAMESPACE_END