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
#include "UtilAll.h"

namespace rocketmq {

ConsumeMessageConcurrentlyService::ConsumeMessageConcurrentlyService(MQConsumer* consumer,
                                                                     int threadCount,
                                                                     MQMessageListener* msgListener)
    : m_pConsumer(consumer), m_pMessageListener(msgListener), m_consumeExecutor(threadCount, false) {
#if !defined(WIN32) && !defined(__APPLE__)
  string taskName = UtilAll::getProcessName();
  prctl(PR_SET_NAME, "ConsumeTP", 0, 0, 0);
#endif
  // start callback threadpool
  m_consumeExecutor.startup();
#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, taskName.c_str(), 0, 0, 0);
#endif
}

ConsumeMessageConcurrentlyService::~ConsumeMessageConcurrentlyService() {
  m_pConsumer = nullptr;
  m_pMessageListener = nullptr;
}

void ConsumeMessageConcurrentlyService::start() {}

void ConsumeMessageConcurrentlyService::shutdown() {
  m_consumeExecutor.shutdown();
}

MessageListenerType ConsumeMessageConcurrentlyService::getConsumeMsgSerivceListenerType() {
  return m_pMessageListener->getMessageListenerType();
}

void ConsumeMessageConcurrentlyService::submitConsumeRequest(PullRequest* request, std::vector<MQMessageExt>& msgs) {
  m_consumeExecutor.submit(std::bind(&ConsumeMessageConcurrentlyService::ConsumeRequest, this, request, msgs));
}

void ConsumeMessageConcurrentlyService::ConsumeRequest(PullRequest* request, std::vector<MQMessageExt>& msgs) {
  if (!request || request->isDroped()) {
    LOG_WARN("the pull result is NULL or Had been dropped");
    request->clearAllMsgs();  // add clear operation to avoid bad state when dropped pullRequest returns normal
    return;
  }

  // empty
  if (msgs.empty()) {
    LOG_WARN("the msg of pull result is EMPTY, its mq:%s", (request->m_messageQueue).toString().c_str());
    return;
  }

  ConsumeStatus status = CONSUME_SUCCESS;
  if (m_pMessageListener != nullptr) {
    resetRetryTopic(msgs);  // set where to sendMessageBack
    request->setLastConsumeTimestamp(UtilAll::currentTimeMillis());
    status = m_pMessageListener->consumeMessage(msgs);
  }

  /*LOG_DEBUG("Consumed MSG size:%d of mq:%s", msgs.size(), (request->m_messageQueue).toString().c_str());*/
  int ackIndex = -1;
  switch (status) {
    case CONSUME_SUCCESS:
      ackIndex = msgs.size();
      break;
    case RECONSUME_LATER:
      ackIndex = -1;
      break;
    default:
      break;
  }

  switch (m_pConsumer->getMessageModel()) {
    case BROADCASTING:
      // Note: broadcasting reconsume should do by application, as it has big affect to broker cluster
      if (ackIndex != (int)msgs.size())
        LOG_WARN("BROADCASTING, the message consume failed, drop it:%s", (request->m_messageQueue).toString().c_str());
      break;
    case CLUSTERING:
      // send back msg to broker;
      for (size_t i = ackIndex + 1; i < msgs.size(); i++) {
        LOG_WARN("consume fail, MQ is:%s, its msgId is:%s, index is:" SIZET_FMT ", reconsume times is:%d",
                 request->m_messageQueue.toString().c_str(), msgs[i].getMsgId().c_str(), i,
                 msgs[i].getReconsumeTimes());
        m_pConsumer->sendMessageBack(msgs[i], 0);
      }
      break;
    default:
      break;
  }

  // update offset
  int64 offset = request->removeMessage(msgs);
  if (offset >= 0) {
    // LOG_DEBUG("update offset:%lld of mq: %s", offset, (request->m_messageQueue).toString().c_str());
    m_pConsumer->updateConsumeOffset(request->m_messageQueue, offset);
  } else {
    LOG_WARN("Note: accumulation consume occurs on mq:%s", (request->m_messageQueue).toString().c_str());
  }
}

void ConsumeMessageConcurrentlyService::resetRetryTopic(vector<MQMessageExt>& msgs) {
  string groupTopic = UtilAll::getRetryTopic(m_pConsumer->getGroupName());
  for (auto& msg : msgs) {
    string retryTopic = msg.getProperty(MQMessage::PROPERTY_RETRY_TOPIC);
    if (!retryTopic.empty() && groupTopic == msg.getTopic()) {
      msg.setTopic(retryTopic);
    }
  }
}

}  // namespace rocketmq
