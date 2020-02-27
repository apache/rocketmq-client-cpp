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
#include "RebalancePullImpl.h"

#include "OffsetStore.h"

namespace rocketmq {


RebalancePullImpl::RebalancePullImpl(DefaultMQPullConsumer* consumer)
    : RebalanceImpl("", CLUSTERING, nullptr, nullptr), m_defaultMQPullConsumer(consumer) {}

bool RebalancePullImpl::removeUnnecessaryMessageQueue(const MQMessageQueue& mq, ProcessQueuePtr pq) {
  // m_defaultMQPullConsumer->getOffsetStore()->persist(mq);
  // m_defaultMQPullConsumer->getOffsetStore()->removeOffset(mq);
  // return true;
  return false;
}

void RebalancePullImpl::removeDirtyOffset(const MQMessageQueue& mq) {
  // m_defaultMQPullConsumer->removeConsumeOffset(mq);
}

int64_t RebalancePullImpl::computePullFromWhere(const MQMessageQueue& mq) {
  return 0;
}

void RebalancePullImpl::dispatchPullRequest(const std::vector<PullRequestPtr>& pullRequestList) {}

void RebalancePullImpl::messageQueueChanged(const std::string& topic,
                                            std::vector<MQMessageQueue>& mqAll,
                                            std::vector<MQMessageQueue>& mqDivided) {}

}  // namespace rocketmq
