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

#include "DefaultMQPushConsumer.h"
#include "Logging.h"
#include "OffsetStore.h"
#include "RebalanceImpl.h"
#include "UtilAll.h"

namespace rocketmq {

const uint64_t ConsumeMessageOrderlyService::MaxTimeConsumeContinuously = 60000;

ConsumeMessageOrderlyService::ConsumeMessageOrderlyService(DefaultMQPushConsumer* consumer,
                                                           int threadCount,
                                                           MQMessageListener* msgListener)
    : m_consumer(consumer),
      m_messageListener(msgListener),
      m_consumeExecutor("OderlyConsumeService", threadCount, false),
      m_scheduledExecutorService(false) {}

ConsumeMessageOrderlyService::~ConsumeMessageOrderlyService() = default;

void ConsumeMessageOrderlyService::start() {
  m_consumeExecutor.startup();

  m_scheduledExecutorService.startup();
  m_scheduledExecutorService.schedule(std::bind(&ConsumeMessageOrderlyService::lockMQPeriodically, this),
                                      ProcessQueue::RebalanceLockInterval, time_unit::milliseconds);
}

void ConsumeMessageOrderlyService::shutdown() {
  stopThreadPool();
  unlockAllMQ();
}

void ConsumeMessageOrderlyService::stopThreadPool() {
  m_consumeExecutor.shutdown();
  m_scheduledExecutorService.shutdown();
}

void ConsumeMessageOrderlyService::lockMQPeriodically() {
  m_consumer->getRebalanceImpl()->lockAll();

  m_scheduledExecutorService.schedule(std::bind(&ConsumeMessageOrderlyService::lockMQPeriodically, this),
                                      ProcessQueue::RebalanceLockInterval, time_unit::milliseconds);
}

void ConsumeMessageOrderlyService::unlockAllMQ() {
  m_consumer->getRebalanceImpl()->unlockAll(false);
}

bool ConsumeMessageOrderlyService::lockOneMQ(const MQMessageQueue& mq) {
  return m_consumer->getRebalanceImpl()->lock(mq);
}

void ConsumeMessageOrderlyService::submitConsumeRequest(std::vector<MQMessageExtPtr2>& msgs,
                                                        ProcessQueuePtr processQueue,
                                                        const MQMessageQueue& messageQueue,
                                                        const bool dispathToConsume) {
  if (dispathToConsume) {
    m_consumeExecutor.submit(
        std::bind(&ConsumeMessageOrderlyService::ConsumeRequest, this, processQueue, messageQueue));
  }
}

void ConsumeMessageOrderlyService::submitConsumeRequestLater(ProcessQueuePtr processQueue,
                                                             const MQMessageQueue& messageQueue,
                                                             const long suspendTimeMillis) {
  long timeMillis = suspendTimeMillis;
  if (timeMillis == -1) {
    timeMillis = 1000;
  }

  timeMillis = std::max(10L, std::min(timeMillis, 30000L));

  static std::vector<MQMessageExtPtr2> dummy;
  m_scheduledExecutorService.schedule(std::bind(&ConsumeMessageOrderlyService::submitConsumeRequest, this,
                                                std::ref(dummy), processQueue, messageQueue, true),
                                      timeMillis, time_unit::milliseconds);
}

void ConsumeMessageOrderlyService::tryLockLaterAndReconsume(const MQMessageQueue& mq,
                                                            ProcessQueuePtr processQueue,
                                                            const long delayMills) {
  m_scheduledExecutorService.schedule(
      [this, mq, processQueue]() {
        bool lockOK = lockOneMQ(mq);
        if (lockOK) {
          submitConsumeRequestLater(processQueue, mq, 10);
        } else {
          submitConsumeRequestLater(processQueue, mq, 3000);
        }
      },
      delayMills, time_unit::milliseconds);
}

void ConsumeMessageOrderlyService::ConsumeRequest(ProcessQueuePtr processQueue, const MQMessageQueue& messageQueue) {
  if (processQueue->isDropped()) {
    LOG_WARN_NEW("run, the message queue not be able to consume, because it's dropped. {}", messageQueue.toString());
    return;
  }

  auto objLock = m_messageQueueLock.fetchLockObject(messageQueue);
  std::lock_guard<std::mutex> lock(*objLock);

  if (BROADCASTING == m_consumer->messageModel() || (processQueue->isLocked() && !processQueue->isLockExpired())) {
    auto beginTime = UtilAll::currentTimeMillis();
    for (bool continueConsume = true; continueConsume;) {
      if (processQueue->isDropped()) {
        LOG_WARN_NEW("the message queue not be able to consume, because it's dropped. {}", messageQueue.toString());
        break;
      }

      if (CLUSTERING == m_consumer->messageModel() && !processQueue->isLocked()) {
        LOG_WARN_NEW("the message queue not locked, so consume later, {}", messageQueue.toString());
        tryLockLaterAndReconsume(messageQueue, processQueue, 10);
        break;
      }

      if (CLUSTERING == m_consumer->messageModel() && !processQueue->isLockExpired()) {
        LOG_WARN_NEW("the message queue lock expired, so consume later, {}", messageQueue.toString());
        tryLockLaterAndReconsume(messageQueue, processQueue, 10);
        break;
      }

      auto interval = UtilAll::currentTimeMillis() - beginTime;
      if (interval > MaxTimeConsumeContinuously) {
        submitConsumeRequestLater(processQueue, messageQueue, 10);
        break;
      }

      const int consumeBatchSize = m_consumer->getConsumeMessageBatchMaxSize();

      std::vector<MQMessageExtPtr2> msgs;
      processQueue->takeMessages(msgs, consumeBatchSize);
      m_consumer->resetRetryTopic(msgs, m_consumer->getGroupName());
      if (!msgs.empty()) {
        ConsumeStatus status = RECONSUME_LATER;
        try {
          std::lock_guard<std::timed_mutex> lock(processQueue->getLockConsume());
          if (processQueue->isDropped()) {
            LOG_WARN_NEW("consumeMessage, the message queue not be able to consume, because it's dropped. {}",
                         messageQueue.toString());
            break;
          }

          status = m_messageListener->consumeMessage(msgs);
        } catch (std::exception& e) {
          // ...
        }

        // processConsumeResult
        long commitOffset = -1L;
        switch (status) {
          case CONSUME_SUCCESS:
            commitOffset = processQueue->commit();
            break;
          case RECONSUME_LATER:
            processQueue->makeMessageToCosumeAgain(msgs);
            submitConsumeRequestLater(processQueue, messageQueue, -1);
            continueConsume = false;
            break;
          default:
            break;
        }

        if (commitOffset >= 0 && !processQueue->isDropped()) {
          m_consumer->getOffsetStore()->updateOffset(messageQueue, commitOffset, false);
        }
      } else {
        continueConsume = false;
      }
    }
  } else {
    if (processQueue->isDropped()) {
      LOG_WARN_NEW("the message queue not be able to consume, because it's dropped. {}", messageQueue.toString());
      return;
    }

    tryLockLaterAndReconsume(messageQueue, processQueue, 100);
  }
}

}  // namespace rocketmq
