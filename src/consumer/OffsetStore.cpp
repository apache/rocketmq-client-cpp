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
#include "OffsetStore.h"

#include <fstream>

#include "Logging.h"
#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "MessageQueue.h"
#include "UtilAll.h"

namespace rocketmq {

//######################################
// LocalFileOffsetStore
//######################################

LocalFileOffsetStore::LocalFileOffsetStore(MQClientInstance* instance, const std::string& groupName)
    : m_clientInstance(instance), m_groupName(groupName) {
  LOG_INFO("new LocalFileOffsetStore");

  std::string clientId = instance->getClientId();
  std::string homeDir(UtilAll::getHomeDirectory());
  std::string storeDir =
      homeDir + FILE_SEPARATOR + ".rocketmq_offsets" + FILE_SEPARATOR + clientId + FILE_SEPARATOR + groupName;
  m_storePath = storeDir + FILE_SEPARATOR + "offsets.json";

  if (!UtilAll::existDirectory(storeDir)) {
    UtilAll::createDirectory(storeDir);
    if (!UtilAll::existDirectory(storeDir)) {
      LOG_ERROR("create offset store dir:%s error", storeDir.c_str());
      std::string errorMsg("create offset store dir failed: ");
      errorMsg.append(storeDir);
      THROW_MQEXCEPTION(MQClientException, errorMsg, -1);
    }
  }
}

LocalFileOffsetStore::~LocalFileOffsetStore() {
  m_clientInstance = nullptr;
  m_offsetTable.clear();
}

void LocalFileOffsetStore::load() {
  auto offsetTable = readLocalOffset();
  if (!offsetTable.empty()) {
    // update offsetTable
    {
      std::lock_guard<std::mutex> lock(m_lock);
      m_offsetTable = offsetTable;
    }

    for (const auto& it : offsetTable) {
      const auto& mq = it.first;
      const auto offset = it.second;
      LOG_INFO_NEW("load consumer's offset, {} {} {}", m_groupName, mq.toString(), offset);
    }
  }
}

void LocalFileOffsetStore::updateOffset(const MQMessageQueue& mq, int64_t offset, bool increaseOnly) {
  std::lock_guard<std::mutex> lock(m_lock);
  m_offsetTable[mq] = offset;
}

int64_t LocalFileOffsetStore::readOffset(const MQMessageQueue& mq, ReadOffsetType type) {
  switch (type) {
    case MEMORY_FIRST_THEN_STORE:
    case READ_FROM_MEMORY: {
      std::lock_guard<std::mutex> lock(m_lock);
      auto it = m_offsetTable.find(mq);
      if (it != m_offsetTable.end()) {
        return it->second;
      } else if (READ_FROM_MEMORY == type) {
        return -1;
      }
    } break;
    case READ_FROM_STORE: {
      auto offsetTable = readLocalOffset();
      if (!offsetTable.empty()) {
        auto it = offsetTable.find(mq);
        if (it != offsetTable.end()) {
          auto offset = it->second;
          updateOffset(mq, offset, false);
          return offset;
        }
      }
    } break;
    default:
      break;
  }
  LOG_ERROR("can not readOffset from offsetStore.json, maybe first time consumation");
  return -1;
}

void LocalFileOffsetStore::persist(const MQMessageQueue& mq) {}

void LocalFileOffsetStore::persistAll(const std::vector<MQMessageQueue>& mqs) {
  if (mqs.empty()) {
    return;
  }

  std::map<MQMessageQueue, int64_t> offsetTable;
  {
    std::lock_guard<std::mutex> lock(m_lock);
    offsetTable = m_offsetTable;
  }

  Json::Value root(Json::objectValue);
  Json::Value jOffsetTable(Json::objectValue);
  for (const auto& mq : mqs) {
    auto it = offsetTable.find(mq);
    if (it != offsetTable.end()) {
      std::string strMQ = RemotingSerializable::toJson(toJson(mq));
      jOffsetTable[strMQ] = Json::Value((Json::Int64)it->second);
    }
  }
  root["offsetTable"] = jOffsetTable;

  std::lock_guard<std::mutex> lock2(m_fileMutex);
  std::string storePathTmp = m_storePath + ".tmp";
  std::ofstream ofstrm(storePathTmp, std::ios::binary | std::ios::out);
  if (ofstrm.is_open()) {
    try {
      RemotingSerializable::toJson(root, ofstrm, true);
    } catch (std::exception& e) {
      THROW_MQEXCEPTION(MQClientException, "persistAll failed", -1);
    }

    if (!UtilAll::ReplaceFile(m_storePath, m_storePath + ".bak") || !UtilAll::ReplaceFile(storePathTmp, m_storePath)) {
      LOG_ERROR("could not rename file: %s", strerror(errno));
    }
  }
}

void LocalFileOffsetStore::removeOffset(const MQMessageQueue& mq) {}

std::map<MQMessageQueue, int64_t> LocalFileOffsetStore::readLocalOffset() {
  std::lock_guard<std::mutex> lock(m_fileMutex);
  std::ifstream ifstrm(m_storePath, std::ios::binary | std::ios::in);
  if (ifstrm.is_open() && !ifstrm.eof()) {
    try {
      Json::Value root = RemotingSerializable::fromJson(ifstrm);
      std::map<MQMessageQueue, int64_t> offsetTable;
      auto& jOffsetTable = root["offsetTable"];
      for (auto& strMQ : jOffsetTable.getMemberNames()) {
        auto& offset = jOffsetTable[strMQ];
        Json::Value jMQ = RemotingSerializable::fromJson(strMQ);
        MQMessageQueue mq(jMQ["topic"].asString(), jMQ["brokerName"].asString(), jMQ["queueId"].asInt());
        offsetTable.emplace(std::move(mq), offset.asInt64());
      }
      return offsetTable;
    } catch (std::exception& e) {
      // ...
    }
  }
  return readLocalOffsetBak();
}

std::map<MQMessageQueue, int64_t> LocalFileOffsetStore::readLocalOffsetBak() {
  std::map<MQMessageQueue, int64_t> offsetTable;
  std::ifstream ifstrm(m_storePath + ".bak", std::ios::binary | std::ios::in);
  if (ifstrm.is_open()) {
    if (!ifstrm.eof()) {
      try {
        Json::Value root = RemotingSerializable::fromJson(ifstrm);
        auto& jOffsetTable = root["offsetTable"];
        for (auto& strMQ : jOffsetTable.getMemberNames()) {
          auto& offset = jOffsetTable[strMQ];
          Json::Value jMQ = RemotingSerializable::fromJson(strMQ);
          MQMessageQueue mq(jMQ["topic"].asString(), jMQ["brokerName"].asString(), jMQ["queueId"].asInt());
          offsetTable.emplace(mq, offset.asInt64());
        }
      } catch (const std::exception& e) {
        LOG_WARN_NEW("readLocalOffset Exception {}", e.what());
        THROW_MQEXCEPTION(MQClientException, "readLocalOffset Exception", -1);
      }
    }
  }
  return offsetTable;
}

//######################################
// RemoteBrokerOffsetStore
//######################################

RemoteBrokerOffsetStore::RemoteBrokerOffsetStore(MQClientInstance* instance, const std::string& groupName)
    : m_clientInstance(instance), m_groupName(groupName) {}

RemoteBrokerOffsetStore::~RemoteBrokerOffsetStore() {
  m_clientInstance = nullptr;
  m_offsetTable.clear();
}

void RemoteBrokerOffsetStore::load() {}

void RemoteBrokerOffsetStore::updateOffset(const MQMessageQueue& mq, int64_t offset, bool increaseOnly) {
  std::lock_guard<std::mutex> lock(m_lock);
  auto it = m_offsetTable.find(mq);
  if (it != m_offsetTable.end()) {
    if (!increaseOnly || offset > it->second) {
      it->second = offset;
    }
  } else {
    m_offsetTable[mq] = offset;
  }
}

int64_t RemoteBrokerOffsetStore::readOffset(const MQMessageQueue& mq, ReadOffsetType type) {
  switch (type) {
    case MEMORY_FIRST_THEN_STORE:
    case READ_FROM_MEMORY: {
      std::lock_guard<std::mutex> lock(m_lock);

      auto it = m_offsetTable.find(mq);
      if (it != m_offsetTable.end()) {
        return it->second;
      } else if (READ_FROM_MEMORY == type) {
        return -1;
      }
    }
    case READ_FROM_STORE: {
      try {
        int64_t brokerOffset = fetchConsumeOffsetFromBroker(mq);
        // update
        updateOffset(mq, brokerOffset, false);
        return brokerOffset;
      } catch (MQBrokerException& e) {
        LOG_ERROR(e.what());
        return -1;
      } catch (MQException& e) {
        LOG_ERROR(e.what());
        return -2;
      }
    }
    default:
      break;
  }
  return -1;
}

void RemoteBrokerOffsetStore::persist(const MQMessageQueue& mq) {
  std::map<MQMessageQueue, int64_t> offsetTable;
  {
    std::lock_guard<std::mutex> lock(m_lock);
    offsetTable = m_offsetTable;
  }

  auto it = offsetTable.find(mq);
  if (it != offsetTable.end()) {
    try {
      updateConsumeOffsetToBroker(mq, it->second);
    } catch (MQException& e) {
      LOG_ERROR("updateConsumeOffsetToBroker error");
    }
  }
}

void RemoteBrokerOffsetStore::persistAll(const std::vector<MQMessageQueue>& mq) {}

void RemoteBrokerOffsetStore::removeOffset(const MQMessageQueue& mq) {
  std::lock_guard<std::mutex> lock(m_lock);
  if (m_offsetTable.find(mq) != m_offsetTable.end()) {
    m_offsetTable.erase(mq);
  }
}

void RemoteBrokerOffsetStore::updateConsumeOffsetToBroker(const MQMessageQueue& mq, int64_t offset) {
  std::unique_ptr<FindBrokerResult> findBrokerResult(m_clientInstance->findBrokerAddressInAdmin(mq.getBrokerName()));

  if (findBrokerResult == nullptr) {
    m_clientInstance->updateTopicRouteInfoFromNameServer(mq.getTopic());
    findBrokerResult.reset(m_clientInstance->findBrokerAddressInAdmin(mq.getBrokerName()));
  }

  if (findBrokerResult != nullptr) {
    UpdateConsumerOffsetRequestHeader* requestHeader = new UpdateConsumerOffsetRequestHeader();
    requestHeader->topic = mq.getTopic();
    requestHeader->consumerGroup = m_groupName;
    requestHeader->queueId = mq.getQueueId();
    requestHeader->commitOffset = offset;

    try {
      LOG_INFO("oneway updateConsumeOffsetToBroker of mq:%s, its offset is:%lld", mq.toString().c_str(), offset);
      return m_clientInstance->getMQClientAPIImpl()->updateConsumerOffsetOneway(findBrokerResult->brokerAddr,
                                                                                requestHeader, 1000 * 5);
    } catch (MQException& e) {
      LOG_ERROR(e.what());
    }
  }
  LOG_WARN("The broker not exist");
}

int64_t RemoteBrokerOffsetStore::fetchConsumeOffsetFromBroker(const MQMessageQueue& mq) {
  std::unique_ptr<FindBrokerResult> findBrokerResult(m_clientInstance->findBrokerAddressInAdmin(mq.getBrokerName()));

  if (findBrokerResult == nullptr) {
    m_clientInstance->updateTopicRouteInfoFromNameServer(mq.getTopic());
    findBrokerResult.reset(m_clientInstance->findBrokerAddressInAdmin(mq.getBrokerName()));
  }

  if (findBrokerResult != nullptr) {
    QueryConsumerOffsetRequestHeader* requestHeader = new QueryConsumerOffsetRequestHeader();
    requestHeader->topic = mq.getTopic();
    requestHeader->consumerGroup = m_groupName;
    requestHeader->queueId = mq.getQueueId();

    return m_clientInstance->getMQClientAPIImpl()->queryConsumerOffset(findBrokerResult->brokerAddr, requestHeader,
                                                                       1000 * 5);
  } else {
    LOG_ERROR("The broker not exist when fetchConsumeOffsetFromBroker");
    THROW_MQEXCEPTION(MQClientException, "The broker not exist", -1);
  }
}

}  // namespace rocketmq
