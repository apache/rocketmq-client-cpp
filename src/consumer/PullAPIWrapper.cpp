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
#include "PullAPIWrapper.h"

#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "MQDecoder.h"
#include "MessageAccessor.h"
#include "PullResultExt.h"
#include "PullSysFlag.h"

namespace rocketmq {

PullAPIWrapper::PullAPIWrapper(MQClientInstance* instance, const std::string& consumerGroup) {
  m_clientInstance = instance;
  m_consumerGroup = consumerGroup;
}

PullAPIWrapper::~PullAPIWrapper() {
  m_clientInstance = nullptr;
  m_pullFromWhichNodeTable.clear();
}

void PullAPIWrapper::updatePullFromWhichNode(const MQMessageQueue& mq, int brokerId) {
  std::lock_guard<std::mutex> lock(m_lock);
  m_pullFromWhichNodeTable[mq] = brokerId;
}

int PullAPIWrapper::recalculatePullFromWhichNode(const MQMessageQueue& mq) {
  std::lock_guard<std::mutex> lock(m_lock);
  if (m_pullFromWhichNodeTable.find(mq) != m_pullFromWhichNodeTable.end()) {
    return m_pullFromWhichNodeTable[mq];
  }
  return MASTER_ID;
}

PullResult PullAPIWrapper::processPullResult(const MQMessageQueue& mq,
                                             PullResult& pullResult,
                                             SubscriptionDataPtr subscriptionData) {
  assert(std::type_index(typeid(pullResult)) == std::type_index(typeid(PullResultExt)));
  auto& pullResultExt = dynamic_cast<PullResultExt&>(pullResult);

  // update node
  updatePullFromWhichNode(mq, pullResultExt.suggestWhichBrokerId);

  std::vector<MQMessageExtPtr2> msgListFilterAgain;
  if (FOUND == pullResultExt.pullStatus) {
    // decode all msg list
    auto msgList = MQDecoder::decodes(*pullResultExt.msgMemBlock);

    // filter msg list again
    if (subscriptionData != nullptr && !subscriptionData->getTagsSet().empty()) {
      msgListFilterAgain.reserve(msgList.size());
      for (const auto& msg : msgList) {
        const auto& msgTag = msg->getTags();
        if (subscriptionData->containTag(msgTag)) {
          msgListFilterAgain.push_back(msg);
        }
      }
    } else {
      msgListFilterAgain.swap(msgList);
    }

    for (auto& msg : msgListFilterAgain) {
      const auto& traFlag = msg->getProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED);
      if (UtilAll::stob(traFlag)) {
        msg->setTransactionId(msg->getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX));
      }
      MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_MIN_OFFSET, UtilAll::to_string(pullResult.minOffset));
      MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_MAX_OFFSET, UtilAll::to_string(pullResult.maxOffset));
    }
  }

  return PullResult(pullResultExt.pullStatus, pullResultExt.nextBeginOffset, pullResultExt.minOffset,
                    pullResultExt.maxOffset, std::move(msgListFilterAgain));
}

PullResult* PullAPIWrapper::pullKernelImpl(const MQMessageQueue& mq,             // 1
                                           std::string subExpression,            // 2
                                           int64_t subVersion,                   // 3
                                           int64_t offset,                       // 4
                                           int maxNums,                          // 5
                                           int sysFlag,                          // 6
                                           int64_t commitOffset,                 // 7
                                           int brokerSuspendMaxTimeMillis,       // 8
                                           int timeoutMillis,                    // 9
                                           CommunicationMode communicationMode,  // 10
                                           PullCallback* pullCallback) {
  std::unique_ptr<FindBrokerResult> findBrokerResult(
      m_clientInstance->findBrokerAddressInSubscribe(mq.getBrokerName(), recalculatePullFromWhichNode(mq), false));
  if (findBrokerResult == nullptr) {
    m_clientInstance->updateTopicRouteInfoFromNameServer(mq.getTopic());
    findBrokerResult.reset(
        m_clientInstance->findBrokerAddressInSubscribe(mq.getBrokerName(), recalculatePullFromWhichNode(mq), false));
  }

  if (findBrokerResult != nullptr) {
    int sysFlagInner = sysFlag;

    if (findBrokerResult->slave) {
      sysFlagInner = PullSysFlag::clearCommitOffsetFlag(sysFlagInner);
    }

    PullMessageRequestHeader* pRequestHeader = new PullMessageRequestHeader();
    pRequestHeader->consumerGroup = m_consumerGroup;
    pRequestHeader->topic = mq.getTopic();
    pRequestHeader->queueId = mq.getQueueId();
    pRequestHeader->queueOffset = offset;
    pRequestHeader->maxMsgNums = maxNums;
    pRequestHeader->sysFlag = sysFlagInner;
    pRequestHeader->commitOffset = commitOffset;
    pRequestHeader->suspendTimeoutMillis = brokerSuspendMaxTimeMillis;
    pRequestHeader->subscription = subExpression;
    pRequestHeader->subVersion = subVersion;

    return m_clientInstance->getMQClientAPIImpl()->pullMessage(findBrokerResult->brokerAddr, pRequestHeader,
                                                                timeoutMillis, communicationMode, pullCallback);
  }

  THROW_MQEXCEPTION(MQClientException, "The broker [" + mq.getBrokerName() + "] not exist", -1);
}

}  // namespace rocketmq
