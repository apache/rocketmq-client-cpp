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
#include "MessageAccessor.hpp"
#include "OffsetStore.h"
#include "UtilAll.h"

namespace rocketmq {

ConsumeMessageConcurrentlyService::ConsumeMessageConcurrentlyService(DefaultMQPushConsumerImpl* consumer,
                                                                     int threadCount,
                                                                     MQMessageListener* msgListener)
    : consumer_(consumer),
      message_listener_(msgListener),
      consume_executor_("ConsumeMessageThread", threadCount, false),
      scheduled_executor_service_("ConsumeMessageScheduledThread", false) {}

ConsumeMessageConcurrentlyService::~ConsumeMessageConcurrentlyService() = default;

void ConsumeMessageConcurrentlyService::start() {
  // start callback threadpool
  consume_executor_.startup();
  scheduled_executor_service_.startup();
}

void ConsumeMessageConcurrentlyService::shutdown() {
  scheduled_executor_service_.shutdown();
  consume_executor_.shutdown();
}

void ConsumeMessageConcurrentlyService::submitConsumeRequest(std::vector<MessageExtPtr>& msgs,
                                                             ProcessQueuePtr processQueue,
                                                             const MQMessageQueue& messageQueue,
                                                             const bool dispathToConsume) {
  consume_executor_.submit(
      std::bind(&ConsumeMessageConcurrentlyService::ConsumeRequest, this, msgs, processQueue, messageQueue));
}

void ConsumeMessageConcurrentlyService::submitConsumeRequestLater(std::vector<MessageExtPtr>& msgs,
                                                                  ProcessQueuePtr processQueue,
                                                                  const MQMessageQueue& messageQueue) {
  scheduled_executor_service_.schedule(
      std::bind(&ConsumeMessageConcurrentlyService::submitConsumeRequest, this, msgs, processQueue, messageQueue, true),
      5000L, time_unit::milliseconds);
}

void ConsumeMessageConcurrentlyService::ConsumeRequest(std::vector<MessageExtPtr>& msgs,
                                                       ProcessQueuePtr processQueue,
                                                       const MQMessageQueue& messageQueue) {
  if (processQueue->dropped()) {
    LOG_WARN_NEW("the message queue not be able to consume, because it's dropped. group={} {}",
                 consumer_->getDefaultMQPushConsumerConfig()->group_name(), messageQueue.toString());
    return;
  }

  // empty
  if (msgs.empty()) {
    LOG_WARN_NEW("the msg of pull result is EMPTY, its mq:{}", messageQueue.toString());
    return;
  }

  consumer_->resetRetryAndNamespace(msgs);  // set where to sendMessageBack

  ConsumeStatus status = RECONSUME_LATER;
  try {
    auto consumeTimestamp = UtilAll::currentTimeMillis();
    processQueue->set_last_consume_timestamp(consumeTimestamp);
    if (!msgs.empty()) {
      auto timestamp = UtilAll::to_string(consumeTimestamp);
      for (const auto& msg : msgs) {
        MessageAccessor::setConsumeStartTimeStamp(*msg, timestamp);
      }
    }
    auto message_list = MQMessageExt::from_list(msgs);
    status = message_listener_->consumeMessage(message_list);
  } catch (const std::exception& e) {
    LOG_WARN_NEW("encounter unexpected exception when consume messages.\n{}", e.what());
  }

  if (processQueue->dropped()) {
    LOG_WARN_NEW("processQueue is dropped without process consume result. messageQueue={}", messageQueue.toString());
    return;
  }

  //
  // processConsumeResult

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

  switch (consumer_->messageModel()) {
    case BROADCASTING:
      // Note: broadcasting reconsume should do by application, as it has big affect to broker cluster
      for (size_t i = ackIndex + 1; i < msgs.size(); i++) {
        const auto& msg = msgs[i];
        LOG_WARN_NEW("BROADCASTING, the message consume failed, drop it, {}", msg->toString());
      }
      break;
    case CLUSTERING: {
      // send back msg to broker
      std::vector<MessageExtPtr> msgBackFailed;
      int idx = ackIndex + 1;
      for (auto iter = msgs.begin() + idx; iter != msgs.end(); idx++) {
        LOG_WARN_NEW("consume fail, MQ is:{}, its msgId is:{}, index is:{}, reconsume times is:{}",
                     messageQueue.toString(), (*iter)->msg_id(), idx, (*iter)->reconsume_times());
        auto& msg = (*iter);
        bool result = consumer_->sendMessageBack(msg, 0, messageQueue.broker_name());
        if (!result) {
          msg->set_reconsume_times(msg->reconsume_times() + 1);
          msgBackFailed.push_back(msg);
          iter = msgs.erase(iter);
        } else {
          iter++;
        }
      }

      if (!msgBackFailed.empty()) {
        // send back failed, reconsume later
        submitConsumeRequestLater(msgBackFailed, processQueue, messageQueue);
      }
    } break;
    default:
      break;
  }

  // update offset
  int64_t offset = processQueue->removeMessage(msgs);
  if (offset >= 0 && !processQueue->dropped()) {
    consumer_->getOffsetStore()->updateOffset(messageQueue, offset, true);
  }
}

}  // namespace rocketmq
