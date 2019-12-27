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
#ifndef __OFFSET_STORE_H__
#define __OFFSET_STORE_H__

#include <map>
#include <mutex>
#include <vector>

#include "MQMessageQueue.h"

namespace rocketmq {

class MQClientInstance;

enum ReadOffsetType {
  // read offset from memory
  READ_FROM_MEMORY,
  // read offset from remoting
  READ_FROM_STORE,
  // read offset from memory firstly, then from remoting
  MEMORY_FIRST_THEN_STORE,
};

class OffsetStore {
 public:
  virtual ~OffsetStore() = default;

  virtual void load() = 0;
  virtual void updateOffset(const MQMessageQueue& mq, int64_t offset, bool increaseOnly) = 0;
  virtual int64_t readOffset(const MQMessageQueue& mq, ReadOffsetType type) = 0;
  virtual void persist(const MQMessageQueue& mq) = 0;
  virtual void persistAll(const std::vector<MQMessageQueue>& mq) = 0;
  virtual void removeOffset(const MQMessageQueue& mq) = 0;
};

class LocalFileOffsetStore : public OffsetStore {
 public:
  LocalFileOffsetStore(MQClientInstance* instance, const std::string& groupName);
  virtual ~LocalFileOffsetStore();

  void load() override;
  void updateOffset(const MQMessageQueue& mq, int64_t offset, bool increaseOnly) override;
  int64_t readOffset(const MQMessageQueue& mq, ReadOffsetType type) override;
  void persist(const MQMessageQueue& mq) override;
  void persistAll(const std::vector<MQMessageQueue>& mq) override;
  void removeOffset(const MQMessageQueue& mq) override;

 private:
  std::map<MQMessageQueue, int64_t> readLocalOffset();
  std::map<MQMessageQueue, int64_t> readLocalOffsetBak();

 private:
  MQClientInstance* m_clientInstance;
  std::string m_groupName;

  std::map<MQMessageQueue, int64_t> m_offsetTable;
  std::mutex m_lock;

  std::string m_storePath;
  std::mutex m_fileMutex;
};

class RemoteBrokerOffsetStore : public OffsetStore {
 public:
  RemoteBrokerOffsetStore(MQClientInstance* instance, const std::string& groupName);
  virtual ~RemoteBrokerOffsetStore();

  void load() override;
  void updateOffset(const MQMessageQueue& mq, int64_t offset, bool increaseOnly) override;
  int64_t readOffset(const MQMessageQueue& mq, ReadOffsetType type) override;
  void persist(const MQMessageQueue& mq) override;
  void persistAll(const std::vector<MQMessageQueue>& mq) override;
  void removeOffset(const MQMessageQueue& mq) override;

 private:
  void updateConsumeOffsetToBroker(const MQMessageQueue& mq, int64_t offset);
  int64_t fetchConsumeOffsetFromBroker(const MQMessageQueue& mq);

 private:
  MQClientInstance* m_clientInstance;
  std::string m_groupName;

  std::map<MQMessageQueue, int64_t> m_offsetTable;
  std::mutex m_lock;
};

}  // namespace rocketmq

#endif  // __OFFSET_STORE_H__
