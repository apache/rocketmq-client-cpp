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

#include "OffsetStore.h"
#include "UtilAll.h"

namespace rocketmq {

RebalancePushImpl::RebalancePushImpl(DefaultMQPushConsumerImpl* consumerImpl)
    : RebalanceImpl(null, CLUSTERING, nullptr, nullptr), default_mq_push_consumer_impl_(consumerImpl) {}

bool RebalancePushImpl::removeUnnecessaryMessageQueue(const MQMessageQueue& mq, ProcessQueuePtr pq) {
  auto* pOffsetStore = default_mq_push_consumer_impl_->getOffsetStore();

  pOffsetStore->persist(mq);
  pOffsetStore->removeOffset(mq);

  if (default_mq_push_consumer_impl_->getMessageListenerType() == messageListenerOrderly &&
      CLUSTERING == default_mq_push_consumer_impl_->messageModel()) {
    try {
      if (UtilAll::try_lock_for(pq->lock_consume(), 1000)) {
        std::lock_guard<std::timed_mutex> lock(pq->lock_consume(), std::adopt_lock);
        // TODO: unlockDelay
        unlock(mq);
        return true;
      } else {
        LOG_WARN("[WRONG]mq is consuming, so can not unlock it, %s. maybe hanged for a while, %ld",
                 mq.toString().c_str(), pq->try_unlock_times());

        pq->inc_try_unlock_times();
      }
    } catch (const std::exception& e) {
      LOG_ERROR("removeUnnecessaryMessageQueue Exception: %s", e.what());
    }

    return false;
  }

  return true;
}

void RebalancePushImpl::removeDirtyOffset(const MQMessageQueue& mq) {
  default_mq_push_consumer_impl_->getOffsetStore()->removeOffset(mq);
}

int64_t RebalancePushImpl::computePullFromWhere(const MQMessageQueue& mq) {
  int64_t result = -1;
  ConsumeFromWhere consumeFromWhere =
      default_mq_push_consumer_impl_->getDefaultMQPushConsumerConfig()->consume_from_where();
  OffsetStore* offsetStore = default_mq_push_consumer_impl_->getOffsetStore();
  switch (consumeFromWhere) {
    case CONSUME_FROM_LAST_OFFSET: {
      int64_t lastOffset = offsetStore->readOffset(mq, ReadOffsetType::READ_FROM_STORE);
      if (lastOffset >= 0) {
        LOG_INFO_NEW("CONSUME_FROM_LAST_OFFSET, lastOffset of mq:{} is {}", mq.toString(), lastOffset);
        result = lastOffset;
      } else if (-1 == lastOffset) {
        LOG_WARN_NEW("CONSUME_FROM_LAST_OFFSET, lastOffset of mq:%s is -1", mq.toString());
        if (UtilAll::isRetryTopic(mq.topic())) {
          LOG_INFO_NEW("CONSUME_FROM_LAST_OFFSET, lastOffset of mq:%s is 0", mq.toString());
          result = 0;
        } else {
          try {
            result = default_mq_push_consumer_impl_->maxOffset(mq);
            LOG_INFO_NEW("CONSUME_FROM_LAST_OFFSET, maxOffset of mq:{} is {}", mq.toString(), result);
          } catch (MQClientException& e) {
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
      int64_t lastOffset = offsetStore->readOffset(mq, ReadOffsetType::READ_FROM_STORE);
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
      int64_t lastOffset = offsetStore->readOffset(mq, ReadOffsetType::READ_FROM_STORE);
      if (lastOffset >= 0) {
        LOG_INFO_NEW("CONSUME_FROM_TIMESTAMP, lastOffset of mq:{} is {}", mq.toString().c_str(), lastOffset);
        result = lastOffset;
      } else if (-1 == lastOffset) {
        if (UtilAll::isRetryTopic(mq.topic())) {
          try {
            result = default_mq_push_consumer_impl_->maxOffset(mq);
            LOG_INFO_NEW("CONSUME_FROM_TIMESTAMP, maxOffset of mq:{} is {}", mq.toString(), result);
          } catch (MQClientException& e) {
            LOG_ERROR_NEW("CONSUME_FROM_TIMESTAMP error, maxOffset of mq:{} is -1", mq.toString());
            result = -1;
          }
        } else {
          try {
            // FIXME: parseDate by YYYYMMDDHHMMSS
            auto timestamp =
                std::stoull(default_mq_push_consumer_impl_->getDefaultMQPushConsumerConfig()->consume_timestamp());
            result = default_mq_push_consumer_impl_->searchOffset(mq, timestamp);
          } catch (MQClientException& e) {
            LOG_ERROR_NEW("CONSUME_FROM_TIMESTAMP error, searchOffset of mq:{}, return 0", mq.toString());
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
    default_mq_push_consumer_impl_->executePullRequestImmediately(pullRequest);
    LOG_INFO_NEW("doRebalance, {}, add a new pull request {}", consumer_group_, pullRequest->toString());
  }
}

void RebalancePushImpl::messageQueueChanged(const std::string& topic,
                                            std::vector<MQMessageQueue>& mqAll,
                                            std::vector<MQMessageQueue>& mqDivided) {
  // TODO: update subscription's version
}

}  // namespace rocketmq
