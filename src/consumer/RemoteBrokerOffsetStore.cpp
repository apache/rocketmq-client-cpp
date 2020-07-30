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
#include "RemoteBrokerOffsetStore.h"

#include "Logging.h"
#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "MessageQueue.hpp"
#include "UtilAll.h"

namespace rocketmq {

RemoteBrokerOffsetStore::RemoteBrokerOffsetStore(MQClientInstance* instance, const std::string& groupName)
    : client_instance_(instance), group_name_(groupName) {}

RemoteBrokerOffsetStore::~RemoteBrokerOffsetStore() {
  client_instance_ = nullptr;
  offset_table_.clear();
}

void RemoteBrokerOffsetStore::load() {}

void RemoteBrokerOffsetStore::updateOffset(const MQMessageQueue& mq, int64_t offset, bool increaseOnly) {
  std::lock_guard<std::mutex> lock(lock_);
  const auto& it = offset_table_.find(mq);
  if (it == offset_table_.end() || !increaseOnly || offset > it->second) {
    offset_table_[mq] = offset;
  }
}

int64_t RemoteBrokerOffsetStore::readOffset(const MQMessageQueue& mq, ReadOffsetType type) {
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
    std::lock_guard<std::mutex> lock(lock_);
    offsetTable = offset_table_;
  }

  const auto& it = offsetTable.find(mq);
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
  std::lock_guard<std::mutex> lock(lock_);
  const auto& it = offset_table_.find(mq);
  if (it != offset_table_.end()) {
    offset_table_.erase(it);
  }
}

void RemoteBrokerOffsetStore::updateConsumeOffsetToBroker(const MQMessageQueue& mq, int64_t offset) {
  std::unique_ptr<FindBrokerResult> findBrokerResult(client_instance_->findBrokerAddressInAdmin(mq.broker_name()));

  if (findBrokerResult == nullptr) {
    client_instance_->updateTopicRouteInfoFromNameServer(mq.topic());
    findBrokerResult.reset(client_instance_->findBrokerAddressInAdmin(mq.broker_name()));
  }

  if (findBrokerResult != nullptr) {
    UpdateConsumerOffsetRequestHeader* requestHeader = new UpdateConsumerOffsetRequestHeader();
    requestHeader->topic = mq.topic();
    requestHeader->consumerGroup = group_name_;
    requestHeader->queueId = mq.queue_id();
    requestHeader->commitOffset = offset;

    try {
      LOG_INFO("oneway updateConsumeOffsetToBroker of mq:%s, its offset is:%lld", mq.toString().c_str(), offset);
      return client_instance_->getMQClientAPIImpl()->updateConsumerOffsetOneway(findBrokerResult->broker_addr(),
                                                                                requestHeader, 1000 * 5);
    } catch (MQException& e) {
      LOG_ERROR(e.what());
    }
  }
  LOG_WARN("The broker not exist");
}

int64_t RemoteBrokerOffsetStore::fetchConsumeOffsetFromBroker(const MQMessageQueue& mq) {
  std::unique_ptr<FindBrokerResult> findBrokerResult(client_instance_->findBrokerAddressInAdmin(mq.broker_name()));

  if (findBrokerResult == nullptr) {
    client_instance_->updateTopicRouteInfoFromNameServer(mq.topic());
    findBrokerResult.reset(client_instance_->findBrokerAddressInAdmin(mq.broker_name()));
  }

  if (findBrokerResult != nullptr) {
    QueryConsumerOffsetRequestHeader* requestHeader = new QueryConsumerOffsetRequestHeader();
    requestHeader->topic = mq.topic();
    requestHeader->consumerGroup = group_name_;
    requestHeader->queueId = mq.queue_id();

    return client_instance_->getMQClientAPIImpl()->queryConsumerOffset(findBrokerResult->broker_addr(), requestHeader,
                                                                       1000 * 5);
  } else {
    LOG_ERROR("The broker not exist when fetchConsumeOffsetFromBroker");
    THROW_MQEXCEPTION(MQClientException, "The broker not exist", -1);
  }
}

}  // namespace rocketmq
