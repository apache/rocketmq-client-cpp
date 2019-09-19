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

#include "Logging.h"
#include "MQClientFactory.h"
#include "MessageQueue.h"
#include "UtilAll.h"

#include <fstream>
#include <sstream>

namespace rocketmq {

//######################################
// SendCallbackWrap
//######################################

OffsetStore::OffsetStore(const string& groupName, MQClientFactory* pfactory)
    : m_groupName(groupName), m_pClientFactory(pfactory) {}

OffsetStore::~OffsetStore() {
  m_pClientFactory = nullptr;
  m_offsetTable.clear();
}

//######################################
// LocalFileOffsetStore
//######################################

class PlainStreamWriterBuilder : public Json::StreamWriterBuilder {
 public:
  PlainStreamWriterBuilder() : StreamWriterBuilder() { (*this)["indentation"] = ""; }
};

static Json::CharReaderBuilder sReaderBuilder;
static Json::StreamWriterBuilder sWriterBuilder;
static PlainStreamWriterBuilder sPlainWriterBuilder;

LocalFileOffsetStore::LocalFileOffsetStore(const string& groupName, MQClientFactory* factory)
    : OffsetStore(groupName, factory) {
  MQConsumer* consumer = factory->selectConsumer(groupName);
  if (consumer != nullptr) {
    LOG_INFO("new LocalFileOffsetStore");

    std::string clientId = UtilAll::getLocalAddress() + "@" + consumer->getInstanceName();
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
}

LocalFileOffsetStore::~LocalFileOffsetStore() {}

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

void LocalFileOffsetStore::updateOffset(const MQMessageQueue& mq, int64 offset) {
  std::lock_guard<std::mutex> lock(m_lock);
  m_offsetTable[mq] = offset;
}

int64 LocalFileOffsetStore::readOffset(const MQMessageQueue& mq,
                                       ReadOffsetType type,
                                       const SessionCredentials& session_credentials) {
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
          updateOffset(mq, offset);
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

void LocalFileOffsetStore::persist(const MQMessageQueue& mq, const SessionCredentials& session_credentials) {}

void LocalFileOffsetStore::persistAll(const std::vector<MQMessageQueue>& mqs) {
  if (mqs.empty()) {
    return;
  }

  std::unique_lock<std::mutex> lock(m_lock);
  auto offsetTable = m_offsetTable;
  lock.unlock();

  Json::Value root(Json::objectValue);
  Json::Value jOffsetTable(Json::objectValue);
  for (const auto& mq : mqs) {
    auto it = offsetTable.find(mq);
    if (it != offsetTable.end()) {
      std::string strMQ = Json::writeString(sPlainWriterBuilder, toJson(mq));
      jOffsetTable[strMQ] = Json::Value(it->second);
    }
  }
  root["offsetTable"] = jOffsetTable;

  std::lock_guard<std::mutex> lock2(m_fileMutex);
  std::string storePathTmp = m_storePath + ".tmp";
  std::ofstream ofstrm(storePathTmp, std::ios::binary | std::ios::out);
  if (ofstrm.is_open()) {
    std::unique_ptr<Json::StreamWriter> writer(sWriterBuilder.newStreamWriter());
    try {
      writer->write(root, &ofstrm);
    } catch (std::exception& e) {
      THROW_MQEXCEPTION(MQClientException, "persistAll failed", -1);
    }

    if (!UtilAll::ReplaceFile(m_storePath, m_storePath + ".bak") || !UtilAll::ReplaceFile(storePathTmp, m_storePath)) {
      LOG_ERROR("could not rename file: %s", strerror(errno));
    }
  }
}

void LocalFileOffsetStore::removeOffset(const MQMessageQueue& mq) {}

LocalFileOffsetStore::MQ2OFFSET LocalFileOffsetStore::readLocalOffset() {
  std::lock_guard<std::mutex> lock(m_fileMutex);
  std::ifstream ifstrm(m_storePath, std::ios::binary | std::ios::in);
  if (ifstrm.is_open() && !ifstrm.eof()) {
    Json::Value root;
    if (Json::parseFromStream(sReaderBuilder, ifstrm, &root, nullptr)) {
      MQ2OFFSET offsetTable;
      auto& jOffsetTable = root["offsetTable"];
      for (auto& strMQ : jOffsetTable.getMemberNames()) {
        auto& offset = jOffsetTable[strMQ];
        std::istringstream isstrm(strMQ);
        Json::Value jMQ;
        if (!Json::parseFromStream(sReaderBuilder, isstrm, &jMQ, nullptr)) {
          MQMessageQueue mq(jMQ["topic"].asString(), jMQ["brokerName"].asString(), jMQ["queueId"].asInt());
          offsetTable.emplace(mq, offset.asInt64());
        }
      }
      return offsetTable;
    }
  }
  return readLocalOffsetBak();
}

LocalFileOffsetStore::MQ2OFFSET LocalFileOffsetStore::readLocalOffsetBak() {
  MQ2OFFSET offsetTable;
  std::ifstream ifstrm(m_storePath + ".bak", std::ios::binary | std::ios::in);
  if (ifstrm.is_open()) {
    if (!ifstrm.eof()) {
      Json::Value root;
      std::string errs;
      if (Json::parseFromStream(sReaderBuilder, ifstrm, &root, &errs)) {
        auto& jOffsetTable = root["offsetTable"];
        for (auto& strMQ : jOffsetTable.getMemberNames()) {
          auto& offset = jOffsetTable[strMQ];
          std::istringstream isstrm(strMQ);
          Json::Value jMQ;
          if (!Json::parseFromStream(sReaderBuilder, isstrm, &jMQ, nullptr)) {
            MQMessageQueue mq(jMQ["topic"].asString(), jMQ["brokerName"].asString(), jMQ["queueId"].asInt());
            offsetTable.emplace(mq, offset.asInt64());
          }
        }
      } else {
        LOG_WARN_NEW("readLocalOffset Exception {}", errs);
        THROW_MQEXCEPTION(MQClientException, "readLocalOffset Exception", -1);
      }
    }
  }
  return offsetTable;
}

//######################################
// RemoteBrokerOffsetStore
//######################################

RemoteBrokerOffsetStore::RemoteBrokerOffsetStore(const string& groupName, MQClientFactory* pfactory)
    : OffsetStore(groupName, pfactory) {}

RemoteBrokerOffsetStore::~RemoteBrokerOffsetStore() {}

void RemoteBrokerOffsetStore::load() {}

void RemoteBrokerOffsetStore::updateOffset(const MQMessageQueue& mq, int64 offset) {
  std::lock_guard<std::mutex> lock(m_lock);
  m_offsetTable[mq] = offset;
}

int64 RemoteBrokerOffsetStore::readOffset(const MQMessageQueue& mq,
                                          ReadOffsetType type,
                                          const SessionCredentials& session_credentials) {
  switch (type) {
    case MEMORY_FIRST_THEN_STORE:
    case READ_FROM_MEMORY: {
      std::lock_guard<std::mutex> lock(m_lock);

      MQ2OFFSET::iterator it = m_offsetTable.find(mq);
      if (it != m_offsetTable.end()) {
        return it->second;
      } else if (READ_FROM_MEMORY == type) {
        return -1;
      }
    }
    case READ_FROM_STORE: {
      try {
        int64 brokerOffset = fetchConsumeOffsetFromBroker(mq, session_credentials);
        //<!update;
        updateOffset(mq, brokerOffset);
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

void RemoteBrokerOffsetStore::persist(const MQMessageQueue& mq, const SessionCredentials& session_credentials) {
  MQ2OFFSET offsetTable;
  {
    std::lock_guard<std::mutex> lock(m_lock);
    offsetTable = m_offsetTable;
  }

  MQ2OFFSET::iterator it = offsetTable.find(mq);
  if (it != offsetTable.end()) {
    try {
      updateConsumeOffsetToBroker(mq, it->second, session_credentials);
    } catch (MQException& e) {
      LOG_ERROR("updateConsumeOffsetToBroker error");
    }
  }
}

void RemoteBrokerOffsetStore::persistAll(const std::vector<MQMessageQueue>& mq) {}

void RemoteBrokerOffsetStore::removeOffset(const MQMessageQueue& mq) {
  std::lock_guard<std::mutex> lock(m_lock);
  if (m_offsetTable.find(mq) != m_offsetTable.end())
    m_offsetTable.erase(mq);
}

void RemoteBrokerOffsetStore::updateConsumeOffsetToBroker(const MQMessageQueue& mq,
                                                          int64 offset,
                                                          const SessionCredentials& session_credentials) {
  std::unique_ptr<FindBrokerResult> pFindBrokerResult(m_pClientFactory->findBrokerAddressInAdmin(mq.getBrokerName()));

  if (pFindBrokerResult == NULL) {
    m_pClientFactory->updateTopicRouteInfoFromNameServer(mq.getTopic(), session_credentials);
    pFindBrokerResult.reset(m_pClientFactory->findBrokerAddressInAdmin(mq.getBrokerName()));
  }

  if (pFindBrokerResult != NULL) {
    UpdateConsumerOffsetRequestHeader* pRequestHeader = new UpdateConsumerOffsetRequestHeader();
    pRequestHeader->topic = mq.getTopic();
    pRequestHeader->consumerGroup = m_groupName;
    pRequestHeader->queueId = mq.getQueueId();
    pRequestHeader->commitOffset = offset;

    try {
      LOG_INFO("oneway updateConsumeOffsetToBroker of mq:%s, its offset is:%lld", mq.toString().c_str(), offset);
      return m_pClientFactory->getMQClientAPIImpl()->updateConsumerOffsetOneway(
          pFindBrokerResult->brokerAddr, pRequestHeader, 1000 * 5, session_credentials);
    } catch (MQException& e) {
      LOG_ERROR(e.what());
    }
  }
  LOG_WARN("The broker not exist");
}

int64 RemoteBrokerOffsetStore::fetchConsumeOffsetFromBroker(const MQMessageQueue& mq,
                                                            const SessionCredentials& session_credentials) {
  std::unique_ptr<FindBrokerResult> pFindBrokerResult(m_pClientFactory->findBrokerAddressInAdmin(mq.getBrokerName()));

  if (pFindBrokerResult == NULL) {
    m_pClientFactory->updateTopicRouteInfoFromNameServer(mq.getTopic(), session_credentials);
    pFindBrokerResult.reset(m_pClientFactory->findBrokerAddressInAdmin(mq.getBrokerName()));
  }

  if (pFindBrokerResult != NULL) {
    QueryConsumerOffsetRequestHeader* pRequestHeader = new QueryConsumerOffsetRequestHeader();
    pRequestHeader->topic = mq.getTopic();
    pRequestHeader->consumerGroup = m_groupName;
    pRequestHeader->queueId = mq.getQueueId();

    return m_pClientFactory->getMQClientAPIImpl()->queryConsumerOffset(pFindBrokerResult->brokerAddr, pRequestHeader,
                                                                       1000 * 5, session_credentials);
  } else {
    LOG_ERROR("The broker not exist when fetchConsumeOffsetFromBroker");
    THROW_MQEXCEPTION(MQClientException, "The broker not exist", -1);
  }
}

}  // namespace rocketmq
