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
#ifndef __LOCK_BATCH_BODY_H__
#define __LOCK_BATCH_BODY_H__

#include <set>
#include <string>

#include "DataBlock.h"
#include "MQMessageQueue.h"
#include "RemotingSerializable.h"
#include "UtilAll.h"

namespace rocketmq {

class LockBatchRequestBody : public RemotingSerializable {
 public:
  std::string getConsumerGroup();
  void setConsumerGroup(std::string consumerGroup);

  std::string getClientId();
  void setClientId(std::string clientId);

  std::vector<MQMessageQueue>& getMqSet();
  void setMqSet(std::vector<MQMessageQueue> mqSet);

  std::string encode() override;

 private:
  std::string m_consumerGroup;
  std::string m_clientId;
  std::vector<MQMessageQueue> m_mqSet;
};

class LockBatchResponseBody {
 public:
  static LockBatchResponseBody* Decode(MemoryBlock& mem);

  const std::vector<MQMessageQueue>& getLockOKMQSet();
  void setLockOKMQSet(std::vector<MQMessageQueue> lockOKMQSet);

 private:
  std::vector<MQMessageQueue> m_lockOKMQSet;
};

class UnlockBatchRequestBody : public RemotingSerializable {
 public:
  std::string getConsumerGroup();
  void setConsumerGroup(std::string consumerGroup);

  std::string getClientId();
  void setClientId(std::string clientId);

  std::vector<MQMessageQueue>& getMqSet();
  void setMqSet(std::vector<MQMessageQueue> mqSet);

  std::string encode() override;

 private:
  std::string m_consumerGroup;
  std::string m_clientId;
  std::vector<MQMessageQueue> m_mqSet;
};

}  // namespace rocketmq

#endif  // __LOCK_BATCH_BODY_H__
