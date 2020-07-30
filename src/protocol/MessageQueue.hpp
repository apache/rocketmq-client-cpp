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
#ifndef ROCKETMQ_PROTOCOL_MESSAGEQUEUE_H_
#define ROCKETMQ_PROTOCOL_MESSAGEQUEUE_H_

#include <json/json.h>

#include "MQMessageQueue.h"

namespace rocketmq {

inline Json::Value toJson(const MQMessageQueue& mq) {
  Json::Value root;
  root["topic"] = mq.topic();
  root["brokerName"] = mq.broker_name();
  root["queueId"] = mq.queue_id();
  return root;
}

}  // namespace rocketmq

#endif  // ROCKETMQ_PROTOCOL_MESSAGEQUEUE_H_
