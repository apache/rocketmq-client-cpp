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
#include "RebalanceLitePullImpl.h"

#include "OffsetStore.h"
#include "UtilAll.h"

namespace rocketmq {

RebalanceLitePullImpl::RebalanceLitePullImpl(DefaultLitePullConsumerImpl* consumerImpl)
    : RebalanceImpl(null, CLUSTERING, nullptr, nullptr), lite_pull_consumer_impl_(consumerImpl) {}

bool RebalanceLitePullImpl::removeUnnecessaryMessageQueue(const MQMessageQueue& mq, ProcessQueuePtr pq) {
  lite_pull_consumer_impl_->getOffsetStore()->persist(mq);
  lite_pull_consumer_impl_->getOffsetStore()->removeOffset(mq);
  return true;
}

void RebalanceLitePullImpl::removeDirtyOffset(const MQMessageQueue& mq) {
  lite_pull_consumer_impl_->getOffsetStore()->removeOffset(mq);
}

int64_t RebalanceLitePullImpl::computePullFromWhere(const MQMessageQueue& mq) {
  int64_t result = -1;
  ConsumeFromWhere consumeFromWhere =
      lite_pull_consumer_impl_->getDefaultLitePullConsumerConfig()->consume_from_where();
  OffsetStore* offsetStore = lite_pull_consumer_impl_->getOffsetStore();
  switch (consumeFromWhere) {
    default:
    case CONSUME_FROM_LAST_OFFSET: {
      long lastOffset = offsetStore->readOffset(mq, ReadOffsetType::MEMORY_FIRST_THEN_STORE);
      if (lastOffset >= 0) {
        result = lastOffset;
      } else if (-1 == lastOffset) {
        if (UtilAll::isRetryTopic(mq.topic())) {  // First start, no offset
          result = 0;
        } else {
          try {
            result = lite_pull_consumer_impl_->maxOffset(mq);
          } catch (MQClientException& e) {
            result = -1;
          }
        }
      } else {
        result = -1;
      }
      break;
    }
    case CONSUME_FROM_FIRST_OFFSET: {
      long lastOffset = offsetStore->readOffset(mq, ReadOffsetType::MEMORY_FIRST_THEN_STORE);
      if (lastOffset >= 0) {
        result = lastOffset;
      } else if (-1 == lastOffset) {
        result = 0L;
      } else {
        result = -1;
      }
      break;
    }
    case CONSUME_FROM_TIMESTAMP: {
      long lastOffset = offsetStore->readOffset(mq, ReadOffsetType::MEMORY_FIRST_THEN_STORE);
      if (lastOffset >= 0) {
        result = lastOffset;
      } else if (-1 == lastOffset) {
        if (UtilAll::isRetryTopic(mq.topic())) {
          try {
            result = lite_pull_consumer_impl_->maxOffset(mq);
          } catch (MQClientException& e) {
            result = -1;
          }
        } else {
          try {
            // FIXME: parseDate by YYYYMMDDHHMMSS
            auto timestamp =
                std::stoull(lite_pull_consumer_impl_->getDefaultLitePullConsumerConfig()->consume_timestamp());
            result = lite_pull_consumer_impl_->searchOffset(mq, timestamp);
          } catch (MQClientException& e) {
            result = -1;
          }
        }
      } else {
        result = -1;
      }
      break;
    }
  }
  return result;
}

void RebalanceLitePullImpl::dispatchPullRequest(const std::vector<PullRequestPtr>& pullRequestList) {}

void RebalanceLitePullImpl::messageQueueChanged(const std::string& topic,
                                                std::vector<MQMessageQueue>& mqAll,
                                                std::vector<MQMessageQueue>& mqDivided) {
  auto* messageQueueListener = lite_pull_consumer_impl_->getMessageQueueListener();
  if (messageQueueListener != nullptr) {
    try {
      messageQueueListener->messageQueueChanged(topic, mqAll, mqDivided);
    } catch (std::exception& e) {
      LOG_ERROR_NEW("messageQueueChanged exception {}", e.what());
    }
  }
}

}  // namespace rocketmq
