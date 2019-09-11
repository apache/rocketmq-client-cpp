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
#include "RebalanceImpl.h"

#include "AllocateMQAveragely.h"
#include "FindBrokerResult.h"
#include "LockBatchBody.h"
#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "OffsetStore.h"

namespace rocketmq {

//######################################
// RebalanceImpl
//######################################

RebalanceImpl::RebalanceImpl(const std::string& consumerGroup,
                             MessageModel messageModel,
                             AllocateMQStrategy* allocateMqStrategy,
                             MQClientInstance* mqClientFactory)
    : m_consumerGroup(consumerGroup),
      m_messageModel(messageModel),
      m_allocateMQStrategy(allocateMqStrategy),
      m_mqClientFactory(mqClientFactory) {}

RebalanceImpl::~RebalanceImpl() {
  for (auto& it : m_subscriptionInner) {
    deleteAndZero(it.second);
  }
}

void RebalanceImpl::unlock(MQMessageQueue mq, const bool oneway) {
  std::unique_ptr<FindBrokerResult> findBrokerResult(
      m_mqClientFactory->findBrokerAddressInSubscribe(mq.getBrokerName(), MASTER_ID, true));
  if (findBrokerResult) {
    std::unique_ptr<UnlockBatchRequestBody> unlockBatchRequest(new UnlockBatchRequestBody());
    unlockBatchRequest->setConsumerGroup(m_consumerGroup);
    unlockBatchRequest->setClientId(m_mqClientFactory->getClientId());
    unlockBatchRequest->getMqSet().push_back(mq);

    try {
      m_mqClientFactory->getMQClientAPIImpl()->unlockBatchMQ(findBrokerResult->brokerAddr, unlockBatchRequest.get(),
                                                             1000);

      ProcessQueuePtr processQueue = getProcessQueue(mq);
      if (processQueue) {
        processQueue->setLocked(false);
        LOG_INFO("the message queue unlock OK, mq:%s", mq.toString().c_str());
      } else {
        LOG_ERROR("the message queue unlock Failed, mq:%s", mq.toString().c_str());
      }
    } catch (MQException& e) {
      LOG_ERROR("unlockBatchMQ exception, mq:%s", mq.toString().c_str());
    }
  } else {
    LOG_WARN("unlock findBrokerAddressInSubscribe ret null for broker:%s", mq.getBrokerName().data());
  }
}

void RebalanceImpl::unlockAll(const bool oneway) {
  auto brokerMqs = buildProcessQueueTableByBrokerName();
  LOG_INFO("unLockAll " SIZET_FMT " broker mqs", brokerMqs->size());

  for (const auto& it : *brokerMqs) {
    const std::string& brokerName = it.first;
    const std::vector<MQMessageQueue>& mqs = it.second;

    if (mqs.size() == 0) {
      continue;
    }

    std::unique_ptr<FindBrokerResult> findBrokerResult(
        m_mqClientFactory->findBrokerAddressInSubscribe(brokerName, MASTER_ID, true));
    if (findBrokerResult) {
      std::unique_ptr<UnlockBatchRequestBody> unlockBatchRequest(new UnlockBatchRequestBody());
      unlockBatchRequest->setConsumerGroup(m_consumerGroup);
      unlockBatchRequest->setClientId(m_mqClientFactory->getClientId());
      unlockBatchRequest->setMqSet(mqs);

      try {
        m_mqClientFactory->getMQClientAPIImpl()->unlockBatchMQ(findBrokerResult->brokerAddr, unlockBatchRequest.get(),
                                                               1000);
        for (const auto& mq : mqs) {
          ProcessQueuePtr processQueue = getProcessQueue(mq);
          if (processQueue) {
            processQueue->setLocked(false);
            LOG_INFO("the message queue unlock OK, mq:%s", mq.toString().c_str());
          } else {
            LOG_ERROR("the message queue unlock Failed, mq:%s", mq.toString().c_str());
          }
        }
      } catch (MQException& e) {
        LOG_ERROR("unlockBatchMQ exception");
      }
    } else {
      LOG_ERROR("unlockAll findBrokerAddressInSubscribe ret null for broker:%s", brokerName.data());
    }
  }
}

std::shared_ptr<BROKER2MQS> RebalanceImpl::buildProcessQueueTableByBrokerName() {
  std::shared_ptr<BROKER2MQS> brokerMqs = std::make_shared<BROKER2MQS>();
  MQ2PQ processQueueTable = getProcessQueueTable();
  for (const auto& it : processQueueTable) {
    const auto& mq = it.first;
    std::string brokerName = mq.getBrokerName();
    if (brokerMqs->find(brokerName) == brokerMqs->end()) {
      brokerMqs->emplace(brokerName, std::vector<MQMessageQueue>());
    }
    (*brokerMqs)[brokerName].push_back(mq);
  }
  return brokerMqs;
}

bool RebalanceImpl::lock(MQMessageQueue mq) {
  std::unique_ptr<FindBrokerResult> findBrokerResult(
      m_mqClientFactory->findBrokerAddressInSubscribe(mq.getBrokerName(), MASTER_ID, true));
  if (findBrokerResult) {
    std::unique_ptr<LockBatchRequestBody> lockBatchRequest(new LockBatchRequestBody());
    lockBatchRequest->setConsumerGroup(m_consumerGroup);
    lockBatchRequest->setClientId(m_mqClientFactory->getClientId());
    lockBatchRequest->getMqSet().push_back(mq);

    try {
      LOG_DEBUG("try to lock mq:%s", mq.toString().c_str());

      std::vector<MQMessageQueue> lockedMq;
      m_mqClientFactory->getMQClientAPIImpl()->lockBatchMQ(findBrokerResult->brokerAddr, lockBatchRequest.get(),
                                                           lockedMq, 1000);

      bool lockOK = false;
      if (!lockedMq.empty()) {
        for (const auto& mmqq : lockedMq) {
          ProcessQueuePtr processQueue = getProcessQueue(mq);
          if (processQueue) {
            processQueue->setLocked(true);
            processQueue->setLastLockTimestamp(UtilAll::currentTimeMillis());
            lockOK = true;
            LOG_INFO("the message queue locked OK, mq:%s", mmqq.toString().c_str());
          } else {
            LOG_WARN("the message queue locked OK, but it is released, mq:%s", mmqq.toString().c_str());
          }
        }

        lockedMq.clear();
      } else {
        LOG_ERROR("the message queue locked Failed, mq:%s", mq.toString().c_str());
      }

      return lockOK;
    } catch (MQException& e) {
      LOG_ERROR("lockBatchMQ exception, mq:%s", mq.toString().c_str());
    }
  } else {
    LOG_ERROR("lock findBrokerAddressInSubscribe ret null for broker:%s", mq.getBrokerName().data());
  }

  return false;
}

void RebalanceImpl::lockAll() {
  auto brokerMqs = buildProcessQueueTableByBrokerName();
  LOG_INFO("LockAll " SIZET_FMT " broker mqs", brokerMqs->size());

  for (const auto& it : *brokerMqs) {
    const std::string& brokerName = it.first;
    const std::vector<MQMessageQueue>& mqs = it.second;

    if (mqs.size() == 0) {
      continue;
    }

    std::unique_ptr<FindBrokerResult> findBrokerResult(
        m_mqClientFactory->findBrokerAddressInSubscribe(brokerName, MASTER_ID, true));
    if (findBrokerResult) {
      std::unique_ptr<LockBatchRequestBody> lockBatchRequest(new LockBatchRequestBody());
      lockBatchRequest->setConsumerGroup(m_consumerGroup);
      lockBatchRequest->setClientId(m_mqClientFactory->getClientId());
      lockBatchRequest->setMqSet(mqs);

      LOG_INFO("try to lock:" SIZET_FMT " mqs of broker:%s", mqs.size(), brokerName.c_str());
      try {
        std::vector<MQMessageQueue> lockOKMQVec;
        m_mqClientFactory->getMQClientAPIImpl()->lockBatchMQ(findBrokerResult->brokerAddr, lockBatchRequest.get(),
                                                             lockOKMQVec, 1000);

        std::set<MQMessageQueue> lockOKMQSet;
        for (const auto& mq : lockOKMQVec) {
          lockOKMQSet.insert(mq);

          ProcessQueuePtr processQueue = getProcessQueue(mq);
          if (processQueue) {
            processQueue->setLocked(true);
            processQueue->setLastLockTimestamp(UtilAll::currentTimeMillis());
            LOG_INFO("the message queue locked OK, mq:%s", mq.toString().c_str());
          } else {
            LOG_WARN("the message queue locked OK, but it is released, mq:%s", mq.toString().c_str());
          }
        }

        for (const auto& mq : mqs) {
          if (lockOKMQSet.find(mq) == lockOKMQSet.end()) {
            ProcessQueuePtr processQueue = getProcessQueue(mq);
            if (processQueue) {
              LOG_WARN("the message queue locked Failed, mq:%s", mq.toString().c_str());
              processQueue->setLocked(false);
            }
          }
        }
      } catch (MQException& e) {
        LOG_ERROR("lockBatchMQ fails");
      }
    } else {
      LOG_ERROR("lockAll findBrokerAddressInSubscribe ret null for broker:%s", brokerName.c_str());
    }
  }
}

void RebalanceImpl::doRebalance(const bool isOrder) throw(MQClientException) {
  LOG_DEBUG("start doRebalance");
  for (const auto& it : m_subscriptionInner) {
    const std::string& topic = it.first;
    LOG_INFO("current topic is:%s", topic.c_str());
    try {
      rebalanceByTopic(topic, isOrder);
    } catch (MQException& e) {
      LOG_ERROR(e.what());
    }
  }

  truncateMessageQueueNotMyTopic();
}

void RebalanceImpl::rebalanceByTopic(const std::string& topic, const bool isOrder) {
  // msg model
  switch (m_messageModel) {
    case BROADCASTING: {
      std::vector<MQMessageQueue> mqSet;
      if (!getTopicSubscribeInfo(topic, mqSet)) {
        bool changed = updateProcessQueueTableInRebalance(topic, mqSet, isOrder);
        if (changed) {
          messageQueueChanged(topic, mqSet, mqSet);
        }
      } else {
        LOG_WARN("doRebalance, %s, but the topic[%s] not exist.", m_consumerGroup.c_str(), topic.c_str());
      }
    } break;
    case CLUSTERING: {
      std::vector<MQMessageQueue> mqAll;
      if (!getTopicSubscribeInfo(topic, mqAll)) {
        if (!UtilAll::isRetryTopic(topic)) {
          LOG_WARN("doRebalance, %s, but the topic[%s] not exist.", m_consumerGroup.c_str(), topic.c_str());
        }
        return;
      }

      std::vector<string> cidAll;
      m_mqClientFactory->findConsumerIds(topic, m_consumerGroup, cidAll);

      if (cidAll.empty()) {
        LOG_WARN("doRebalance, %s %s, get consumer id list failed", m_consumerGroup.c_str(), topic.c_str());
        return;
      }

      // log
      for (auto& cid : cidAll) {
        LOG_INFO("client id:%s of topic:%s", cid.c_str(), topic.c_str());
      }

      // sort
      sort(mqAll.begin(), mqAll.end());
      sort(cidAll.begin(), cidAll.end());

      // allocate mqs
      std::vector<MQMessageQueue> allocateResult;
      try {
        m_allocateMQStrategy->allocate(m_mqClientFactory->getClientId(), mqAll, cidAll, allocateResult);
      } catch (MQException& e) {
        LOG_ERROR("AllocateMessageQueueStrategy.allocate Exception: %s", e.what());
        return;
      }

      // update local
      bool changed = updateProcessQueueTableInRebalance(topic, allocateResult, isOrder);
      if (changed) {
        LOG_INFO("rebalanced result changed. group=%s, topic=%s, clientId=%s, mqAllSize=" SIZET_FMT
                 ", cidAllSize=" SIZET_FMT ", rebalanceResultSize=" SIZET_FMT ", rebalanceResultSet:",
                 m_consumerGroup.c_str(), topic.c_str(), m_mqClientFactory->getClientId().c_str(), mqAll.size(),
                 cidAll.size(), allocateResult.size());
        for (auto& mq : allocateResult) {
          LOG_INFO("allocate mq:%s", mq.toString().c_str());
        }
        messageQueueChanged(topic, mqAll, allocateResult);
      }
    } break;
    default:
      break;
  }
}

void RebalanceImpl::truncateMessageQueueNotMyTopic() {
  auto& subTable = getSubscriptionInner();
  std::vector<MQMessageQueue> mqs = getAllocatedMQ();
  for (const auto& mq : mqs) {
    if (subTable.find(mq.getTopic()) == subTable.end()) {
      auto pq = removeProcessQueueDirectly(mq);
      if (pq != nullptr) {
        pq->setDropped(true);
        LOG_INFO("doRebalance, %s, truncateMessageQueueNotMyTopic remove unnecessary mq, {}", m_consumerGroup.c_str(),
                 mq.toString().c_str());
      }
    }
  }
}

bool RebalanceImpl::updateProcessQueueTableInRebalance(const std::string& topic,
                                                       std::vector<MQMessageQueue>& mqSet,
                                                       const bool isOrder) {
  LOG_DEBUG("updateRequestTableInRebalance Enter");

  bool changed = false;

  // remove
  MQ2PQ processQueueTable(getProcessQueueTable());  // get copy of m_processQueueTable
  for (const auto& it : processQueueTable) {
    const auto& mq = it.first;
    auto pq = it.second;

    if (mq.getTopic() == topic) {
      if (mqSet.empty() || (find(mqSet.begin(), mqSet.end(), mq) == mqSet.end())) {
        pq->setDropped(true);
        if (removeUnnecessaryMessageQueue(mq, pq)) {
          removeProcessQueueDirectly(mq);
          changed = true;
          LOG_INFO("doRebalance, %s, remove unnecessary mq, %s", m_consumerGroup.c_str(), mq.toString().c_str());
        }
      } else if (pq->isPullExpired()) {
        switch (consumeType()) {
          case CONSUME_ACTIVELY:
            break;
          case CONSUME_PASSIVELY:
            pq->setDropped(true);
            if (removeUnnecessaryMessageQueue(mq, pq)) {
              removeProcessQueueDirectly(mq);
              changed = true;
              LOG_ERROR("[BUG]doRebalance, %s, remove unnecessary mq, %s, because pull is pause, so try to fixed it",
                        m_consumerGroup.c_str(), mq.toString().c_str());
            }
            break;
          default:
            break;
        }
      }
    }
  }

  // update
  std::vector<PullRequestPtr> pullRequestList;
  for (const auto& mq : mqSet) {
    ProcessQueuePtr pq = getProcessQueue(mq);
    if (nullptr == pq) {
      if (isOrder && !lock(mq)) {
        LOG_WARN("doRebalance, %s, add a new mq failed, %s, because lock failed", m_consumerGroup.c_str(),
                 mq.toString().c_str());
        continue;
      }

      removeDirtyOffset(mq);
      pq.reset(new ProcessQueue());
      int64_t nextOffset = computePullFromWhere(mq);
      if (nextOffset >= 0) {
        auto pre = putProcessQueueIfAbsent(mq, pq);
        if (pre) {
          LOG_INFO("doRebalance, %s, mq already exists, %s", m_consumerGroup.c_str(), mq.toString().c_str());
        } else {
          LOG_INFO("doRebalance, %s, add a new mq, %s", m_consumerGroup.c_str(), mq.toString().c_str());
          PullRequestPtr pullRequest(new PullRequest());
          pullRequest->setConsumerGroup(m_consumerGroup);
          pullRequest->setNextOffset(nextOffset);
          pullRequest->setMessageQueue(mq);
          pullRequest->setProcessQueue(pq);
          pullRequestList.push_back(std::move(pullRequest));
          changed = true;
        }
      } else {
        LOG_WARN("doRebalance, %s, add new mq failed, %s", m_consumerGroup.c_str(), mq.toString().c_str());
      }
    }
  }

  dispatchPullRequest(pullRequestList);

  LOG_DEBUG("updateRequestTableInRebalance exit");
  return changed;
}

void RebalanceImpl::removeProcessQueue(const MQMessageQueue& mq) {
  std::lock_guard<std::mutex> lock(m_processQueueTableMutex);
  auto it = m_processQueueTable.find(mq);
  if (it != m_processQueueTable.end()) {
    ProcessQueuePtr prev = it->second;
    m_processQueueTable.erase(it);

    bool dropped = prev->isDropped();
    prev->setDropped(true);
    removeUnnecessaryMessageQueue(mq, prev);
    LOG_INFO("Fix Offset, {}, remove unnecessary mq, {} Dropped: {}", m_consumerGroup, mq.toString(),
             UtilAll::to_string(dropped));
  }
}

ProcessQueuePtr RebalanceImpl::removeProcessQueueDirectly(const MQMessageQueue& mq) {
  std::lock_guard<std::mutex> lock(m_processQueueTableMutex);
  auto it = m_processQueueTable.find(mq);
  if (it != m_processQueueTable.end()) {
    ProcessQueuePtr old = it->second;
    m_processQueueTable.erase(it);
    return old;
  }
  return ProcessQueuePtr();
}

ProcessQueuePtr RebalanceImpl::putProcessQueueIfAbsent(const MQMessageQueue& mq, ProcessQueuePtr pq) {
  std::lock_guard<std::mutex> lock(m_processQueueTableMutex);
  auto it = m_processQueueTable.find(mq);
  if (it != m_processQueueTable.end()) {
    return it->second;
  } else {
    m_processQueueTable[mq] = pq;
    return ProcessQueuePtr();
  }
}

ProcessQueuePtr RebalanceImpl::getProcessQueue(const MQMessageQueue& mq) {
  std::lock_guard<std::mutex> lock(m_processQueueTableMutex);
  if (m_processQueueTable.find(mq) != m_processQueueTable.end()) {
    return m_processQueueTable[mq];
  } else {
    ProcessQueuePtr ptr;
    return ptr;
  }
}

MQ2PQ RebalanceImpl::getProcessQueueTable() {
  std::lock_guard<std::mutex> lock(m_processQueueTableMutex);
  return m_processQueueTable;
}

std::vector<MQMessageQueue> RebalanceImpl::getAllocatedMQ() {
  std::vector<MQMessageQueue> mqs;
  std::lock_guard<std::mutex> lock(m_processQueueTableMutex);
  for (const auto& it : m_processQueueTable) {
    mqs.push_back(it.first);
  }
  return mqs;
}

void RebalanceImpl::destroy() {
  std::lock_guard<std::mutex> lock(m_processQueueTableMutex);
  for (const auto& it : m_processQueueTable) {
    it.second->setDropped(true);
  }

  m_processQueueTable.clear();
}

TOPIC2SD& RebalanceImpl::getSubscriptionInner() {
  return m_subscriptionInner;
}

SubscriptionDataPtr RebalanceImpl::getSubscriptionData(const string& topic) {
  auto it = m_subscriptionInner.find(topic);
  if (it != m_subscriptionInner.end()) {
    return it->second;
  }
  return nullptr;
}

void RebalanceImpl::setSubscriptionData(const std::string& topic, SubscriptionDataPtr subscriptionData) noexcept {
  if (subscriptionData != nullptr) {
    auto it = m_subscriptionInner.find(topic);
    if (it != m_subscriptionInner.end()) {
      deleteAndZero(it->second);
    }
    m_subscriptionInner[topic] = subscriptionData;
  }
}

bool RebalanceImpl::getTopicSubscribeInfo(const string& topic, std::vector<MQMessageQueue>& mqs) {
  std::lock_guard<std::mutex> lock(m_topicSubscribeInfoTableMutex);
  if (m_topicSubscribeInfoTable.find(topic) != m_topicSubscribeInfoTable.end()) {
    mqs = m_topicSubscribeInfoTable[topic];
    return true;
  }
  return false;
}

void RebalanceImpl::setTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& mqs) {
  if (m_subscriptionInner.find(topic) == m_subscriptionInner.end()) {
    return;
  }

  {
    std::lock_guard<std::mutex> lock(m_topicSubscribeInfoTableMutex);
    if (m_topicSubscribeInfoTable.find(topic) != m_topicSubscribeInfoTable.end())
      m_topicSubscribeInfoTable.erase(topic);
    m_topicSubscribeInfoTable[topic] = mqs;
  }

  // log
  for (const auto& mq : mqs) {
    LOG_DEBUG("topic [%s] has :%s", topic.c_str(), mq.toString().c_str());
  }
}

//######################################
// RebalancePush
//######################################

RebalancePushImpl::RebalancePushImpl(DefaultMQPushConsumer* consumer)
    : RebalanceImpl("", CLUSTERING, nullptr, nullptr), m_defaultMQPushConsumer(consumer) {}

bool RebalancePushImpl::removeUnnecessaryMessageQueue(const MQMessageQueue& mq, ProcessQueuePtr pq) {
  auto* pOffsetStore = m_defaultMQPushConsumer->getOffsetStore();

  pOffsetStore->persist(mq);
  pOffsetStore->removeOffset(mq);

  if (m_defaultMQPushConsumer->getMessageListenerType() == messageListenerOrderly &&
      CLUSTERING == m_defaultMQPushConsumer->getMessageModel()) {
    try {
      if (UtilAll::try_lock_for(pq->getLockConsume(), 1000)) {
        std::lock_guard<std::timed_mutex> lock(pq->getLockConsume(), std::adopt_lock);
        // TODO: unlockDelay
        unlock(mq);
        return true;
      } else {
        LOG_WARN("[WRONG]mq is consuming, so can not unlock it, %s. maybe hanged for a while, %ld",
                 mq.toString().c_str(), pq->getTryUnlockTimes());

        pq->incTryUnlockTimes();
      }
    } catch (const std::exception& e) {
      LOG_ERROR("removeUnnecessaryMessageQueue Exception: %s", e.what());
    }

    return false;
  }

  return true;
}

void RebalancePushImpl::removeDirtyOffset(const MQMessageQueue& mq) {
  m_defaultMQPushConsumer->getOffsetStore()->removeOffset(mq);
}

int64_t RebalancePushImpl::computePullFromWhere(const MQMessageQueue& mq) {
  int64_t result = -1;
  ConsumeFromWhere consumeFromWhere = m_defaultMQPushConsumer->getConsumeFromWhere();
  OffsetStore* pOffsetStore = m_defaultMQPushConsumer->getOffsetStore();
  switch (consumeFromWhere) {
    case CONSUME_FROM_LAST_OFFSET: {
      int64_t lastOffset = pOffsetStore->readOffset(mq, READ_FROM_STORE);
      if (lastOffset >= 0) {
        LOG_INFO_NEW("CONSUME_FROM_LAST_OFFSET, lastOffset of mq:{} is {}", mq.toString(), lastOffset);
        result = lastOffset;
      } else if (-1 == lastOffset) {
        LOG_WARN_NEW("CONSUME_FROM_LAST_OFFSET, lastOffset of mq:%s is -1", mq.toString());
        if (UtilAll::isRetryTopic(mq.getTopic())) {
          LOG_INFO_NEW("CONSUME_FROM_LAST_OFFSET, lastOffset of mq:%s is 0", mq.toString());
          result = 0;
        } else {
          try {
            result = m_defaultMQPushConsumer->maxOffset(mq);
            LOG_INFO_NEW("CONSUME_FROM_LAST_OFFSET, maxOffset of mq:{} is {}", mq.toString(), result);
          } catch (MQException& e) {
            LOG_ERROR_NEW("CONSUME_FROM_LAST_OFFSET error, lastOffset of mq:{} is -1", mq.toString());
            result = -1;
          }
        }
      } else {
        LOG_ERROR_NEW("CONSUME_FROM_LAST_OFFSET error, lastOffset  of mq:{} is -1", mq.toString());
        result = -1;
      }
    } break;
    case CONSUME_FROM_FIRST_OFFSET: {
      int64_t lastOffset = pOffsetStore->readOffset(mq, READ_FROM_STORE);
      if (lastOffset >= 0) {
        LOG_INFO_NEW("CONSUME_FROM_FIRST_OFFSET, lastOffset of mq:{} is {}", mq.toString(), lastOffset);
        result = lastOffset;
      } else if (-1 == lastOffset) {
        LOG_INFO_NEW("CONSUME_FROM_FIRST_OFFSET, lastOffset of mq:{}, return 0", mq.toString());
        result = 0;
      } else {
        LOG_INFO_NEW("CONSUME_FROM_FIRST_OFFSET, lastOffset of mq:{}, return -1", mq.toString());
        result = -1;
      }
    } break;
    case CONSUME_FROM_TIMESTAMP: {
      int64_t lastOffset = pOffsetStore->readOffset(mq, READ_FROM_STORE);
      if (lastOffset >= 0) {
        LOG_INFO_NEW("CONSUME_FROM_TIMESTAMP, lastOffset of mq:{} is {}", mq.toString().c_str(), lastOffset);
        result = lastOffset;
      } else if (-1 == lastOffset) {
        if (UtilAll::isRetryTopic(mq.getTopic())) {
          try {
            result = m_defaultMQPushConsumer->maxOffset(mq);
            LOG_INFO_NEW("CONSUME_FROM_TIMESTAMP, maxOffset of mq:{} is {}", mq.toString(), result);
          } catch (MQException& e) {
            LOG_ERROR_NEW("CONSUME_FROM_TIMESTAMP error, lastOffset of mq:{} is -1", mq.toString());
            result = -1;
          }
        } else {
          try {
          } catch (MQException& e) {
            LOG_ERROR_NEW("CONSUME_FROM_TIMESTAMP error, lastOffset of mq:{}, return 0", mq.toString());
            result = -1;
          }
        }
      } else {
        LOG_ERROR_NEW("CONSUME_FROM_TIMESTAMP error, lastOffset of mq:{}, return -1", mq.toString());
        result = -1;
      }
    } break;
    default:
      break;
  }
  return result;
}

void RebalancePushImpl::dispatchPullRequest(const std::vector<PullRequestPtr>& pullRequestList) {
  for (const auto& pullRequest : pullRequestList) {
    m_defaultMQPushConsumer->executePullRequestImmediately(pullRequest);
    LOG_INFO("doRebalance, %s, add a new pull request %s", m_consumerGroup.c_str(), pullRequest->toString().c_str());
  }
}

void RebalancePushImpl::messageQueueChanged(const string& topic,
                                            std::vector<MQMessageQueue>& mqAll,
                                            std::vector<MQMessageQueue>& mqDivided) {
  // TODO: update subscription's version
}

//######################################
// RebalancePull
//######################################

RebalancePullImpl::RebalancePullImpl(DefaultMQPullConsumer* consumer)
    : RebalanceImpl("", CLUSTERING, nullptr, nullptr), m_defaultMQPullConsumer(consumer) {}

bool RebalancePullImpl::removeUnnecessaryMessageQueue(const MQMessageQueue& mq, ProcessQueuePtr pq) {
  m_defaultMQPullConsumer->getOffsetStore()->persist(mq);
  m_defaultMQPullConsumer->getOffsetStore()->removeOffset(mq);
  return true;
}

void RebalancePullImpl::removeDirtyOffset(const MQMessageQueue& mq) {
  m_defaultMQPullConsumer->removeConsumeOffset(mq);
}

int64_t RebalancePullImpl::computePullFromWhere(const MQMessageQueue& mq) {
  return 0;
}

void RebalancePullImpl::dispatchPullRequest(const std::vector<PullRequestPtr>& pullRequestList) {}

void RebalancePullImpl::messageQueueChanged(const std::string& topic,
                                            std::vector<MQMessageQueue>& mqAll,
                                            std::vector<MQMessageQueue>& mqDivided) {}

}  // namespace rocketmq
