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
#ifndef ROCKETMQ_MQMESSAGEQUEUE_H_
#define ROCKETMQ_MQMESSAGEQUEUE_H_

#include <string>  // std::string

#include "RocketMQClient.h"

namespace rocketmq {

/**
 * MessageQueue(Topic, BrokerName, QueueId)
 */
class ROCKETMQCLIENT_API MQMessageQueue {
 public:
  MQMessageQueue();
  MQMessageQueue(const std::string& topic, const std::string& brokerName, int queueId);

  MQMessageQueue(const MQMessageQueue& other);
  MQMessageQueue& operator=(const MQMessageQueue& other);

  bool operator==(const MQMessageQueue& mq) const;
  bool operator!=(const MQMessageQueue& mq) const { return !operator==(mq); }

  bool operator<(const MQMessageQueue& mq) const;
  int compareTo(const MQMessageQueue& mq) const;

  std::string toString() const;

 public:
  inline const std::string& topic() const { return topic_; };
  inline void set_topic(const std::string& topic) { topic_ = topic; };

  inline const std::string& broker_name() const { return broker_name_; };
  inline void set_broker_name(const std::string& broker_name) { broker_name_ = broker_name; };

  inline int queue_id() const { return queue_id_; };
  inline void set_queue_id(int queue_id) { queue_id_ = queue_id; };

 private:
  std::string topic_;
  std::string broker_name_;
  int queue_id_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQMESSAGEQUEUE_H_
