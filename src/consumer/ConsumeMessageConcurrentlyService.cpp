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
#include "MessageAccessor.h"
#include "OffsetStore.h"
#include "UtilAll.h"

namespace rocketmq {

ConsumeMessageConcurrentlyService::ConsumeMessageConcurrentlyService(DefaultMQPushConsumer* consumer,
                                                                     int threadCount,
                                                                     MQMessageListener* msgListener)
    : m_consumer(consumer), m_messageListener(msgListener), m_consumeExecutor("ConsumeService", threadCount, false) {}

ConsumeMessageConcurrentlyService::~ConsumeMessageConcurrentlyService() = default;

void ConsumeMessageConcurrentlyService::start() {
  // start callback threadpool
  m_consumeExecutor.startup();
}

void ConsumeMessageConcurrentlyService::shutdown() {
  m_consumeExecutor.shutdown();
}

void ConsumeMessageConcurrentlyService::submitConsumeRequest(std::vector<MQMessageExtPtr2>& msgs,
                                                             ProcessQueuePtr processQueue,
                                                             const MQMessageQueue& messageQueue,
                                                             const bool dispathToConsume) {
  m_consumeExecutor.submit(
      std::bind(&ConsumeMessageConcurrentlyService::ConsumeRequest, this, msgs, processQueue, messageQueue));
}

void ConsumeMessageConcurrentlyService::ConsumeRequest(std::vector<MQMessageExtPtr2>& msgs,
                                                       ProcessQueuePtr processQueue,
                                                       const MQMessageQueue& messageQueue) {
  if (processQueue->isDropped()) {
    LOG_WARN_NEW("the message queue not be able to consume, because it's dropped. group={} {}",
                 m_consumer->getGroupName(), messageQueue.toString());
    return;
  }

  // empty
  if (msgs.empty()) {
    LOG_WARN_NEW("the msg of pull result is EMPTY, its mq:{}", messageQueue.toString());
    return;
  }

  m_consumer->resetRetryTopic(msgs, m_consumer->getGroupName());  // set where to sendMessageBack

  ConsumeStatus status = RECONSUME_LATER;
  try {
    auto consumeTimestamp = UtilAll::currentTimeMillis();
    processQueue->setLastConsumeTimestamp(consumeTimestamp);
    if (!msgs.empty()) {
      auto timestamp = UtilAll::to_string(consumeTimestamp);
      for (auto& msg : msgs) {
        MessageAccessor::setConsumeStartTimeStamp(*msg, timestamp);
      }
    }
    status = m_messageListener->consumeMessage(msgs);
  } catch (std::exception& e) {
    // ...
  }

  // processConsumeResult
  if (!processQueue->isDropped()) {
    int ackIndex = -1;
    switch (status) {
      case CONSUME_SUCCESS:
        ackIndex = msgs.size() - 1;
        break;
      case RECONSUME_LATER:
        ackIndex = -1;
        break;
      default:
        break;
    }

    switch (m_consumer->messageModel()) {
      case BROADCASTING:
        // Note: broadcasting reconsume should do by application, as it has big affect to broker cluster
        for (size_t i = ackIndex + 1; i < msgs.size(); i++) {
          const auto& msg = msgs[i];
          LOG_WARN_NEW("BROADCASTING, the message consume failed, drop it, {}", msg->toString());
        }
        break;
      case CLUSTERING: {
        // send back msg to broker
        std::vector<MQMessageExtPtr2> msgBackFailed;
        for (size_t i = ackIndex + 1; i < msgs.size(); i++) {
          LOG_WARN_NEW("consume fail, MQ is:{}, its msgId is:{}, index is:{}, reconsume times is:{}",
                       messageQueue.toString(), msgs[i]->getMsgId(), i, msgs[i]->getReconsumeTimes());
          auto& msg = msgs[i];
          bool result = m_consumer->sendMessageBack(*msg, 0);
          if (!result) {
            msg->setReconsumeTimes(msg->getReconsumeTimes() + 1);
            msgBackFailed.push_back(msg);
          }
        }

        if (!msgBackFailed.empty()) {
          // FIXME: send back failed
          // m_pConsumer->submitConsumeRequestLater()
        }
      } break;
      default:
        break;
    }

    // update offset
    int64_t offset = processQueue->removeMessage(msgs);
    if (offset >= 0 && !processQueue->isDropped()) {
      m_consumer->getOffsetStore()->updateOffset(messageQueue, offset, true);
    }
  }
}

}  // namespace rocketmq
