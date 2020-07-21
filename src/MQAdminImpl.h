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
#ifndef ROCKETMQ_MQADMINIMPL_H_
#define ROCKETMQ_MQADMINIMPL_H_

#include <string>
#include <vector>

#include "MQClientInstance.h"
#include "MQMessageExt.h"
#include "MQMessageQueue.h"
#include "QueryResult.h"

namespace rocketmq {

class MQAdminImpl {
 public:
  MQAdminImpl(MQClientInstance* clientInstance) : client_instance_(clientInstance) {}

  void createTopic(const std::string& key, const std::string& newTopic, int queueNum);

  void fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs);

  int64_t searchOffset(const MQMessageQueue& mq, int64_t timestamp);
  int64_t maxOffset(const MQMessageQueue& mq);
  int64_t minOffset(const MQMessageQueue& mq);
  int64_t earliestMsgStoreTime(const MQMessageQueue& mq);

  MQMessageExt viewMessage(const std::string& msgId);
  QueryResult queryMessage(const std::string& topic, const std::string& key, int maxNum, int64_t begin, int64_t end);

 private:
  MQClientInstance* client_instance_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQADMINIMPL_H_
