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

#include "Logging.h"
#include "OffsetStore.h"
#include "RebalanceImpl.h"
#include "UtilAll.h"

namespace rocketmq {

const uint64_t MAX_TIME_CONSUME_CONTINUOUSLY = 60000;

ConsumeMessageOrderlyService::ConsumeMessageOrderlyService(DefaultMQPushConsumerImpl* consumer,
                                                           int threadCount,
                                                           MQMessageListener* msgListener)
    : consumer_(consumer),
      message_listener_(msgListener),
      consume_executor_("OderlyConsumeService", threadCount, false),
      scheduled_executor_service_(false) {}

ConsumeMessageOrderlyService::~ConsumeMessageOrderlyService() = default;

void ConsumeMessageOrderlyService::start() {
  consume_executor_.startup();

  scheduled_executor_service_.startup();
  scheduled_executor_service_.schedule(std::bind(&ConsumeMessageOrderlyService::lockMQPeriodically, this),
                                       ProcessQueue::REBALANCE_LOCK_INTERVAL, time_unit::milliseconds);
}

void ConsumeMessageOrderlyService::shutdown() {
  stopThreadPool();
  unlockAllMQ();
}

void ConsumeMessageOrderlyService::stopThreadPool() {
  consume_executor_.shutdown();
  scheduled_executor_service_.shutdown();
}

void ConsumeMessageOrderlyService::lockMQPeriodically() {
  consumer_->getRebalanceImpl()->lockAll();

  scheduled_executor_service_.schedule(std::bind(&ConsumeMessageOrderlyService::lockMQPeriodically, this),
                                       ProcessQueue::REBALANCE_LOCK_INTERVAL, time_unit::milliseconds);
}

void ConsumeMessageOrderlyService::unlockAllMQ() {
  consumer_->getRebalanceImpl()->unlockAll(false);
}

bool ConsumeMessageOrderlyService::lockOneMQ(const MQMessageQueue& mq) {
  return consumer_->getRebalanceImpl()->lock(mq);
}

void ConsumeMessageOrderlyService::submitConsumeRequest(std::vector<MessageExtPtr>& msgs,
                                                        ProcessQueuePtr processQueue,
                                                        const MQMessageQueue& messageQueue,
                                                        const bool dispathToConsume) {
  if (dispathToConsume) {
    consume_executor_.submit(
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

  static std::vector<MessageExtPtr> dummy;
  scheduled_executor_service_.schedule(std::bind(&ConsumeMessageOrderlyService::submitConsumeRequest, this,
                                                 std::ref(dummy), processQueue, messageQueue, true),
                                       timeMillis, time_unit::milliseconds);
}

void ConsumeMessageOrderlyService::tryLockLaterAndReconsume(const MQMessageQueue& mq,
                                                            ProcessQueuePtr processQueue,
                                                            const long delayMills) {
  scheduled_executor_service_.schedule(
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
  if (processQueue->dropped()) {
    LOG_WARN_NEW("run, the message queue not be able to consume, because it's dropped. {}", messageQueue.toString());
    return;
  }

  auto objLock = message_queue_lock_.fetchLockObject(messageQueue);
  std::lock_guard<std::mutex> lock(*objLock);

  if (BROADCASTING == consumer_->messageModel() || (processQueue->locked() && !processQueue->isLockExpired())) {
    auto beginTime = UtilAll::currentTimeMillis();
    for (bool continueConsume = true; continueConsume;) {
      if (processQueue->dropped()) {
        LOG_WARN_NEW("the message queue not be able to consume, because it's dropped. {}", messageQueue.toString());
        break;
      }

      if (CLUSTERING == consumer_->messageModel() && !processQueue->locked()) {
        LOG_WARN_NEW("the message queue not locked, so consume later, {}", messageQueue.toString());
        tryLockLaterAndReconsume(messageQueue, processQueue, 10);
        break;
      }

      if (CLUSTERING == consumer_->messageModel() && processQueue->isLockExpired()) {
        LOG_WARN_NEW("the message queue lock expired, so consume later, {}", messageQueue.toString());
        tryLockLaterAndReconsume(messageQueue, processQueue, 10);
        break;
      }

      auto interval = UtilAll::currentTimeMillis() - beginTime;
      if (interval > MAX_TIME_CONSUME_CONTINUOUSLY) {
        submitConsumeRequestLater(processQueue, messageQueue, 10);
        break;
      }

      const int consumeBatchSize = consumer_->getDefaultMQPushConsumerConfig()->consume_message_batch_max_size();

      std::vector<MessageExtPtr> msgs;
      processQueue->takeMessages(msgs, consumeBatchSize);
      consumer_->resetRetryAndNamespace(msgs);
      if (!msgs.empty()) {
        ConsumeStatus status = RECONSUME_LATER;
        try {
          std::lock_guard<std::timed_mutex> lock(processQueue->lock_consume());
          if (processQueue->dropped()) {
            LOG_WARN_NEW("consumeMessage, the message queue not be able to consume, because it's dropped. {}",
                         messageQueue.toString());
            break;
          }
          auto message_list = MQMessageExt::from_list(msgs);
          status = message_listener_->consumeMessage(message_list);
        } catch (const std::exception& e) {
          LOG_WARN_NEW("encounter unexpected exception when consume messages.\n{}", e.what());
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

        if (commitOffset >= 0 && !processQueue->dropped()) {
          consumer_->getOffsetStore()->updateOffset(messageQueue, commitOffset, false);
        }
      } else {
        continueConsume = false;
      }
    }
  } else {
    if (processQueue->dropped()) {
      LOG_WARN_NEW("the message queue not be able to consume, because it's dropped. {}", messageQueue.toString());
      return;
    }

    tryLockLaterAndReconsume(messageQueue, processQueue, 100);
  }
}

}  // namespace rocketmq
