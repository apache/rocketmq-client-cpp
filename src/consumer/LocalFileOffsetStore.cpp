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
#include "LocalFileOffsetStore.h"

#include <fstream>

#include "Logging.h"
#include "MQClientInstance.h"
#include "MessageQueue.hpp"
#include "UtilAll.h"

namespace rocketmq {

LocalFileOffsetStore::LocalFileOffsetStore(MQClientInstance* instance, const std::string& groupName)
    : client_instance_(instance), group_name_(groupName) {
  LOG_INFO("new LocalFileOffsetStore");

  std::string clientId = instance->getClientId();
  std::string homeDir(UtilAll::getHomeDirectory());
  std::string storeDir =
      homeDir + FILE_SEPARATOR + ".rocketmq_offsets" + FILE_SEPARATOR + clientId + FILE_SEPARATOR + groupName;
  store_path_ = storeDir + FILE_SEPARATOR + "offsets.json";

  if (!UtilAll::existDirectory(storeDir)) {
    UtilAll::createDirectory(storeDir);
    if (!UtilAll::existDirectory(storeDir)) {
      LOG_ERROR_NEW("create offset store directory failed: {}", storeDir);
      std::string errorMsg("create offset store directory failed: ");
      errorMsg.append(storeDir);
      THROW_MQEXCEPTION(MQClientException, errorMsg, -1);
    }
  }
}

LocalFileOffsetStore::~LocalFileOffsetStore() {
  client_instance_ = nullptr;
  offset_table_.clear();
}

void LocalFileOffsetStore::load() {
  auto offsetTable = readLocalOffset();
  if (!offsetTable.empty()) {
    // update offsetTable
    {
      std::lock_guard<std::mutex> lock(lock_);
      offset_table_ = offsetTable;
    }

    for (const auto& it : offsetTable) {
      const auto& mq = it.first;
      const auto offset = it.second;
      LOG_INFO_NEW("load consumer's offset, {} {} {}", group_name_, mq.toString(), offset);
    }
  }
}

void LocalFileOffsetStore::updateOffset(const MQMessageQueue& mq, int64_t offset, bool increaseOnly) {
  std::lock_guard<std::mutex> lock(lock_);
  offset_table_[mq] = offset;
}

int64_t LocalFileOffsetStore::readOffset(const MQMessageQueue& mq, ReadOffsetType type) {
  switch (type) {
    case MEMORY_FIRST_THEN_STORE:
    case READ_FROM_MEMORY: {
      std::lock_guard<std::mutex> lock(lock_);
      const auto& it = offset_table_.find(mq);
      if (it != offset_table_.end()) {
        return it->second;
      } else if (READ_FROM_MEMORY == type) {
        return -1;
      }
    } break;
    case READ_FROM_STORE: {
      auto offsetTable = readLocalOffset();
      if (!offsetTable.empty()) {
        const auto& it = offsetTable.find(mq);
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

void LocalFileOffsetStore::persistAll(std::vector<MQMessageQueue>& mqs) {
  if (mqs.empty()) {
    return;
  }

  std::map<MQMessageQueue, int64_t> offsetTable;
  {
    std::lock_guard<std::mutex> lock(lock_);
    offsetTable = offset_table_;
  }

  Json::Value root(Json::objectValue);
  Json::Value jOffsetTable(Json::objectValue);
  for (const auto& mq : mqs) {
    const auto& it = offsetTable.find(mq);
    if (it != offsetTable.end()) {
      std::string strMQ = RemotingSerializable::toJson(toJson(mq));
      jOffsetTable[strMQ] = Json::Value((Json::Int64)it->second);
    }
  }
  root["offsetTable"] = jOffsetTable;

  std::lock_guard<std::mutex> lock2(file_mutex_);
  std::string storePathTmp = store_path_ + ".tmp";
  std::ofstream ofstrm(storePathTmp, std::ios::binary | std::ios::out);
  if (ofstrm.is_open()) {
    try {
      RemotingSerializable::toJson(root, ofstrm, true);
    } catch (std::exception& e) {
      THROW_MQEXCEPTION(MQClientException, "persistAll failed", -1);
    }

    if (!UtilAll::ReplaceFile(store_path_, store_path_ + ".bak") || !UtilAll::ReplaceFile(storePathTmp, store_path_)) {
      LOG_ERROR("could not rename file: %s", strerror(errno));
    }
  }
}

void LocalFileOffsetStore::removeOffset(const MQMessageQueue& mq) {}

std::map<MQMessageQueue, int64_t> LocalFileOffsetStore::readLocalOffset() {
  std::lock_guard<std::mutex> lock(file_mutex_);
  std::ifstream ifstrm(store_path_, std::ios::binary | std::ios::in);
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
  std::ifstream ifstrm(store_path_ + ".bak", std::ios::binary | std::ios::in);
  if (ifstrm.is_open()) {
    if (!ifstrm.eof()) {
      try {
        Json::Value root = RemotingSerializable::fromJson(ifstrm);
        auto& jOffsetTable = root["offsetTable"];
        for (auto& strMQ : jOffsetTable.getMemberNames()) {
          auto& offset = jOffsetTable[strMQ];
          Json::Value jMQ = RemotingSerializable::fromJson(strMQ);
          MQMessageQueue mq(jMQ["topic"].asString(), jMQ["brokerName"].asString(), jMQ["queueId"].asInt());
          offsetTable.emplace(std::move(mq), offset.asInt64());
        }
      } catch (const std::exception& e) {
        LOG_WARN_NEW("readLocalOffset Exception {}", e.what());
        THROW_MQEXCEPTION(MQClientException, "readLocalOffset Exception", -1);
      }
    }
  }
  return offsetTable;
}

}  // namespace rocketmq
