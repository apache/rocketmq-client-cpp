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
#include "RebalancePushImpl.h"

#include "UtilAll.h"
#include "OffsetStore.h"

namespace rocketmq {

RebalancePushImpl::RebalancePushImpl(DefaultMQPushConsumerImpl* consumer)
    : RebalanceImpl("", CLUSTERING, nullptr, nullptr), m_defaultMQPushConsumer(consumer) {}

bool RebalancePushImpl::removeUnnecessaryMessageQueue(const MQMessageQueue& mq, ProcessQueuePtr pq) {
  auto* pOffsetStore = m_defaultMQPushConsumer->getOffsetStore();

  pOffsetStore->persist(mq);
  pOffsetStore->removeOffset(mq);

  if (m_defaultMQPushConsumer->getMessageListenerType() == messageListenerOrderly &&
      CLUSTERING == m_defaultMQPushConsumer->messageModel()) {
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
  ConsumeFromWhere consumeFromWhere = m_defaultMQPushConsumer->consumeFromWhere();
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

void RebalancePushImpl::messageQueueChanged(const std::string& topic,
                                            std::vector<MQMessageQueue>& mqAll,
                                            std::vector<MQMessageQueue>& mqDivided) {
  // TODO: update subscription's version
}

}  // namespace rocketmq
