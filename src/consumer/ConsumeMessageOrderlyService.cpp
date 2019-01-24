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
#include "ConsumeMsgService.h"

#if !defined(WIN32) && !defined(__APPLE__)
#include <sys/prctl.h>
#endif

#include "DefaultMQPushConsumer.h"
#include "Logging.h"
#include "Rebalance.h"
#include "UtilAll.h"

namespace rocketmq {

ConsumeMessageOrderlyService::ConsumeMessageOrderlyService(MQConsumer* consumer,
                                                           int threadCount,
                                                           MQMessageListener* msgListener)
    : m_pConsumer(consumer),
      m_shutdownInprogress(false),
      m_pMessageListener(msgListener),
      m_MaxTimeConsumeContinuously(60 * 1000),
      m_consumeExecutor(threadCount, false),
      m_scheduledExecutorService(false) {
#if !defined(WIN32) && !defined(__APPLE__)
  string taskName = UtilAll::getProcessName();
  prctl(PR_SET_NAME, "oderlyConsumeTP", 0, 0, 0);
#endif
  m_consumeExecutor.startup();
#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, taskName.c_str(), 0, 0, 0);
#endif
}

ConsumeMessageOrderlyService::~ConsumeMessageOrderlyService(void) {
  m_pConsumer = nullptr;
  m_pMessageListener = nullptr;
}

void ConsumeMessageOrderlyService::start() {
  m_scheduledExecutorService.startup();
  m_scheduledExecutorService.schedule(std::bind(&ConsumeMessageOrderlyService::lockMQPeriodically, this),
                                      PullRequest::RebalanceLockInterval, time_unit::milliseconds);
}

void ConsumeMessageOrderlyService::shutdown() {
  stopThreadPool();
  unlockAllMQ();
}

void ConsumeMessageOrderlyService::lockMQPeriodically() {
  m_pConsumer->getRebalance()->lockAll();

  m_scheduledExecutorService.schedule(std::bind(&ConsumeMessageOrderlyService::lockMQPeriodically, this),
                                      PullRequest::RebalanceLockInterval, time_unit::milliseconds);
}

void ConsumeMessageOrderlyService::unlockAllMQ() {
  m_pConsumer->getRebalance()->unlockAll(false);
}

bool ConsumeMessageOrderlyService::lockOneMQ(const MQMessageQueue& mq) {
  return m_pConsumer->getRebalance()->lock(mq);
}

void ConsumeMessageOrderlyService::stopThreadPool() {
  m_shutdownInprogress = true;

  m_consumeExecutor.shutdown();
  m_scheduledExecutorService.shutdown();
}

MessageListenerType ConsumeMessageOrderlyService::getConsumeMsgServiceListenerType() {
  return m_pMessageListener->getMessageListenerType();
}

void ConsumeMessageOrderlyService::submitConsumeRequest(std::shared_ptr<PullRequest> request,
                                                        std::vector<MQMessageExt>& msgs) {
  m_consumeExecutor.submit(std::bind(&ConsumeMessageOrderlyService::ConsumeRequest, this, request));
}

void ConsumeMessageOrderlyService::submitConsumeRequestLater(std::shared_ptr<PullRequest> request, bool tryLockMQ) {
  LOG_INFO("submit consumeRequest later for mq:%s", request->m_messageQueue.toString().c_str());
  std::vector<MQMessageExt> msgs;
  submitConsumeRequest(request, msgs);
  if (tryLockMQ) {
    lockOneMQ(request->m_messageQueue);
  }
}

void ConsumeMessageOrderlyService::ConsumeRequest(std::shared_ptr<PullRequest> request) {
  bool bGetMutex = false;
  std::unique_lock<std::timed_mutex> lock(request->getPullRequestCriticalSection(), std::try_to_lock);
  if (!lock.owns_lock()) {
    if (!lock.try_lock_for(std::chrono::seconds(1))) {
      LOG_ERROR("ConsumeRequest of:%s get timed_mutex timeout", request->m_messageQueue.toString().c_str());
      return;
    } else {
      bGetMutex = true;
    }
  } else {
    bGetMutex = true;
  }
  if (!bGetMutex) {
    // LOG_INFO("pullrequest of mq:%s consume inprogress",
    // request->m_messageQueue.toString().c_str());
    return;
  }
  if (!request || request->isDroped()) {
    LOG_WARN("the pull result is NULL or Had been dropped");
    // add clear operation to avoid bad state when dropped pullRequest returns normal
    request->clearAllMsgs();
    return;
  }

  if (m_pMessageListener) {
    if ((request->isLocked() && !request->isLockExpired()) || m_pConsumer->getMessageModel() == BROADCASTING) {
      DefaultMQPushConsumer* pConsumer = (DefaultMQPushConsumer*)m_pConsumer;
      uint64_t beginTime = UtilAll::currentTimeMillis();
      bool continueConsume = true;
      while (continueConsume) {
        if ((UtilAll::currentTimeMillis() - beginTime) > m_MaxTimeConsumeContinuously) {
          LOG_INFO("continuely consume message queue:%s more than 60s, consume it later",
                   request->m_messageQueue.toString().c_str());
          tryLockLaterAndReconsume(request, false);
          break;
        }
        std::vector<MQMessageExt> msgs;
        request->takeMessages(msgs, pConsumer->getConsumeMessageBatchMaxSize());
        if (!msgs.empty()) {
          request->setLastConsumeTimestamp(UtilAll::currentTimeMillis());
          ConsumeStatus consumeStatus = m_pMessageListener->consumeMessage(msgs);
          if (consumeStatus == RECONSUME_LATER) {
            request->makeMessageToCosumeAgain(msgs);
            continueConsume = false;
            tryLockLaterAndReconsume(request, false);
          } else {
            m_pConsumer->updateConsumeOffset(request->m_messageQueue, request->commit());
          }
        } else {
          continueConsume = false;
        }
        msgs.clear();
        if (m_shutdownInprogress) {
          LOG_INFO("shutdown inprogress, break the consuming");
          return;
        }
      }
      LOG_DEBUG("consume once exit of mq:%s", request->m_messageQueue.toString().c_str());
    } else {
      LOG_ERROR("message queue:%s was not locked", request->m_messageQueue.toString().c_str());
      tryLockLaterAndReconsume(request, true);
    }
  }
}

void ConsumeMessageOrderlyService::tryLockLaterAndReconsume(std::shared_ptr<PullRequest> request, bool tryLockMQ) {
  int retryTimer = tryLockMQ ? 500 : 100;

  m_scheduledExecutorService.schedule(
      std::bind(&ConsumeMessageOrderlyService::submitConsumeRequestLater, this, request, tryLockMQ), retryTimer,
      time_unit::milliseconds);
}

}  // namespace rocketmq
