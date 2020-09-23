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
#ifndef ROCKETMQ_CONSUMER_REMOTEBROKEROFFSETSTORE_H_
#define ROCKETMQ_CONSUMER_REMOTEBROKEROFFSETSTORE_H_

#include <map>    // std::map
#include <mutex>  // std::mutex

#include "MQClientInstance.h"
#include "OffsetStore.h"

namespace rocketmq {

class RemoteBrokerOffsetStore : public OffsetStore {
 public:
  RemoteBrokerOffsetStore(MQClientInstance* instance, const std::string& groupName);
  virtual ~RemoteBrokerOffsetStore();

  void load() override;
  void updateOffset(const MQMessageQueue& mq, int64_t offset, bool increaseOnly) override;
  int64_t readOffset(const MQMessageQueue& mq, ReadOffsetType type) override;
  void persist(const MQMessageQueue& mq) override;
  void persistAll(std::vector<MQMessageQueue>& mqs) override;
  void removeOffset(const MQMessageQueue& mq) override;

 private:
  void updateConsumeOffsetToBroker(const MQMessageQueue& mq, int64_t offset);
  int64_t fetchConsumeOffsetFromBroker(const MQMessageQueue& mq);

 private:
  MQClientInstance* client_instance_;
  std::string group_name_;

  std::map<MQMessageQueue, int64_t> offset_table_;
  std::mutex lock_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_CONSUMER_REMOTEBROKEROFFSETSTORE_H_
