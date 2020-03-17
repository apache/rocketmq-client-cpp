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
#if !defined(WIN32) && !defined(__APPLE__)
#include <sys/prctl.h>
#endif

#include "ConsumeMsgService.h"
#include "DefaultMQPushConsumer.h"
#include "Logging.h"
#include "MessageAccessor.h"
#include "UtilAll.h"

namespace rocketmq {

//<!************************************************************************
ConsumeMessageConcurrentlyService::ConsumeMessageConcurrentlyService(MQConsumer* consumer,
                                                                     int threadCount,
                                                                     MQMessageListener* msgListener)
    : m_pConsumer(consumer), m_pMessageListener(msgListener), m_ioServiceWork(m_ioService) {
#if !defined(WIN32) && !defined(__APPLE__)
  string taskName = UtilAll::getProcessName();
  prctl(PR_SET_NAME, "ConsumeTP", 0, 0, 0);
#endif
  for (int i = 0; i != threadCount; ++i) {
    m_threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &m_ioService));
  }
#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, taskName.c_str(), 0, 0, 0);
#endif
}

ConsumeMessageConcurrentlyService::~ConsumeMessageConcurrentlyService(void) {
  m_pConsumer = NULL;
  m_pMessageListener = NULL;
}

void ConsumeMessageConcurrentlyService::start() {}

void ConsumeMessageConcurrentlyService::shutdown() {
  stopThreadPool();
}

void ConsumeMessageConcurrentlyService::stopThreadPool() {
  m_ioService.stop();
  m_threadpool.join_all();
}

MessageListenerType ConsumeMessageConcurrentlyService::getConsumeMsgSerivceListenerType() {
  return m_pMessageListener->getMessageListenerType();
}

void ConsumeMessageConcurrentlyService::submitConsumeRequest(boost::weak_ptr<PullRequest> pullRequest,
                                                             vector<MQMessageExt>& msgs) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_WARN("Pull request has been released");
    return;
  }
  if (request->isDropped()) {
    LOG_INFO("Pull request for %s is dropped, which will be released in next re-balance.",
             request->m_messageQueue.toString().c_str());
    return;
  }
  if (!request->isDropped() && !m_ioService.stopped()) {
    m_ioService.post(boost::bind(&ConsumeMessageConcurrentlyService::ConsumeRequest, this, request, msgs));
  } else {
    LOG_INFO("IOService stopped or Pull request for %s is dropped, will not post ConsumeRequest.",
             request->m_messageQueue.toString().c_str());
  }
}

void ConsumeMessageConcurrentlyService::submitConsumeRequestLater(boost::weak_ptr<PullRequest> pullRequest,
                                                                  vector<MQMessageExt>& msgs,
                                                                  int millis) {
  if (msgs.empty()) {
    return;
  }
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_WARN("Pull request has been released");
    return;
  }
  if (request->isDropped()) {
    LOG_INFO("Pull request is set as dropped with mq:%s, need release in next rebalance.",
             (request->m_messageQueue).toString().c_str());
    return;
  }
  if (!request->isDropped() && !m_ioService.stopped()) {
    boost::asio::deadline_timer* t =
        new boost::asio::deadline_timer(m_ioService, boost::posix_time::milliseconds(millis));
    t->async_wait(
        boost::bind(&(ConsumeMessageConcurrentlyService::static_submitConsumeRequest), this, t, request, msgs));
    LOG_INFO("Submit Message to Consumer [%s] Later and Sleep [%d]ms.", (request->m_messageQueue).toString().c_str(),
             millis);
  } else {
    LOG_INFO("IOService stopped or Pull request for %s is dropped, will not post delay ConsumeRequest.",
             request->m_messageQueue.toString().c_str());
  }
}

void ConsumeMessageConcurrentlyService::static_submitConsumeRequest(void* context,
                                                                    boost::asio::deadline_timer* t,
                                                                    boost::weak_ptr<PullRequest> pullRequest,
                                                                    vector<MQMessageExt>& msgs) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_WARN("Pull request has been released");
    return;
  }
  ConsumeMessageConcurrentlyService* pService = (ConsumeMessageConcurrentlyService*)context;
  if (pService) {
    pService->triggersubmitConsumeRequestLater(t, request, msgs);
  }
}

void ConsumeMessageConcurrentlyService::triggersubmitConsumeRequestLater(boost::asio::deadline_timer* t,
                                                                         boost::weak_ptr<PullRequest> pullRequest,
                                                                         vector<MQMessageExt>& msgs) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_WARN("Pull request has been released");
    return;
  }
  submitConsumeRequest(request, msgs);
  deleteAndZero(t);
}

void ConsumeMessageConcurrentlyService::ConsumeRequest(boost::weak_ptr<PullRequest> pullRequest,
                                                       vector<MQMessageExt>& msgs) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_WARN("Pull request has been released");
    return;
  }
  if (request->isDropped()) {
    LOG_WARN("the pull request for %s Had been dropped before", request->m_messageQueue.toString().c_str());
    request->clearAllMsgs();  // add clear operation to avoid bad state when
    // dropped pullRequest returns normal
    return;
  }
  if (msgs.empty()) {
    LOG_WARN("the msg of pull result is NULL,its mq:%s", (request->m_messageQueue).toString().c_str());
    return;
  }
  ConsumeMessageContext consumeMessageContext;
  DefaultMQPushConsumerImpl* pConsumer = dynamic_cast<DefaultMQPushConsumerImpl*>(m_pConsumer);
  if (pConsumer) {
    if (pConsumer->getMessageTrace() && pConsumer->hasConsumeMessageHook()) {
      consumeMessageContext.setDefaultMQPushConsumer(pConsumer);
      consumeMessageContext.setConsumerGroup(pConsumer->getGroupName());
      consumeMessageContext.setMessageQueue(request->m_messageQueue);
      consumeMessageContext.setMsgList(msgs);
      consumeMessageContext.setSuccess(false);
      consumeMessageContext.setNameSpace(pConsumer->getNameSpace());
      pConsumer->executeConsumeMessageHookBefore(&consumeMessageContext);
    }
  }
  ConsumeStatus status = CONSUME_SUCCESS;
  if (m_pMessageListener != NULL) {
    resetRetryTopic(msgs);
    request->setLastConsumeTimestamp(UtilAll::currentTimeMillis());
    LOG_DEBUG("=====Receive Messages,Topic[%s], MsgId[%s],Body[%s],RetryTimes[%d]", msgs[0].getTopic().c_str(),
              msgs[0].getMsgId().c_str(), msgs[0].getBody().c_str(), msgs[0].getReconsumeTimes());
    if (m_pConsumer->isUseNameSpaceMode()) {
      MessageAccessor::withoutNameSpace(msgs, m_pConsumer->getNameSpace());
    }

    if (pConsumer->getMessageTrace() && pConsumer->hasConsumeMessageHook()) {
      // For open trace message, consume message one by one.
      for (size_t i = 0; i < msgs.size(); ++i) {
        LOG_DEBUG("=====Trace Receive Messages,Topic[%s], MsgId[%s],Body[%s],RetryTimes[%d]",
                  msgs[i].getTopic().c_str(), msgs[i].getMsgId().c_str(), msgs[i].getBody().c_str(),
                  msgs[i].getReconsumeTimes());
        std::vector<MQMessageExt> msgInner;
        msgInner.push_back(msgs[i]);
        if (status != CONSUME_SUCCESS) {
          // all the Messages behind should be set to failed.
          status = RECONSUME_LATER;
          consumeMessageContext.setMsgIndex(i);
          consumeMessageContext.setStatus("RECONSUME_LATER");
          consumeMessageContext.setSuccess(false);
          pConsumer->executeConsumeMessageHookAfter(&consumeMessageContext);
          continue;
        }
        try {
          status = m_pMessageListener->consumeMessage(msgInner);
        } catch (...) {
          status = RECONSUME_LATER;
          LOG_ERROR("Consumer's code is buggy. Un-caught exception raised");
        }
        consumeMessageContext.setMsgIndex(i);  // indicate message position,not support batch consumer
        if (status == CONSUME_SUCCESS) {
          consumeMessageContext.setStatus("CONSUME_SUCCESS");
          consumeMessageContext.setSuccess(true);
        } else {
          status = RECONSUME_LATER;
          consumeMessageContext.setStatus("RECONSUME_LATER");
          consumeMessageContext.setSuccess(false);
        }
        pConsumer->executeConsumeMessageHookAfter(&consumeMessageContext);
      }
    } else {
      try {
        status = m_pMessageListener->consumeMessage(msgs);
      } catch (...) {
        status = RECONSUME_LATER;
        LOG_ERROR("Consumer's code is buggy. Un-caught exception raised");
      }
    }
  }

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

  std::vector<MQMessageExt> localRetryMsgs;
  switch (m_pConsumer->getMessageModel()) {
    case BROADCASTING: {
      // Note: broadcasting reconsume should do by application, as it has big
      // affect to broker cluster
      if (ackIndex != (int)msgs.size())
        LOG_WARN("BROADCASTING, the message consume failed, drop it:%s", (request->m_messageQueue).toString().c_str());
      break;
    }
    case CLUSTERING: {
      // send back msg to broker;
      for (size_t i = ackIndex + 1; i < msgs.size(); i++) {
        LOG_DEBUG("consume fail, MQ is:%s, its msgId is:%s, index is:" SIZET_FMT ", reconsume times is:%d",
                  (request->m_messageQueue).toString().c_str(), msgs[i].getMsgId().c_str(), i,
                  msgs[i].getReconsumeTimes());
        if (m_pConsumer->getConsumeType() == CONSUME_PASSIVELY) {
          string brokerName = request->m_messageQueue.getBrokerName();
          if (m_pConsumer->isUseNameSpaceMode()) {
            MessageAccessor::withNameSpace(msgs[i], m_pConsumer->getNameSpace());
          }
          if (!m_pConsumer->sendMessageBack(msgs[i], 0, brokerName)) {
            LOG_WARN("Send message back fail, MQ is:%s, its msgId is:%s, index is:%d, re-consume times is:%d",
                     (request->m_messageQueue).toString().c_str(), msgs[i].getMsgId().c_str(), i,
                     msgs[i].getReconsumeTimes());
            msgs[i].setReconsumeTimes(msgs[i].getReconsumeTimes() + 1);
            localRetryMsgs.push_back(msgs[i]);
          }
        }
      }
      break;
    }
    default:
      break;
  }

  if (!localRetryMsgs.empty()) {
    LOG_ERROR("Client side re-consume launched due to both message consuming and SDK send-back retry failure");
    for (std::vector<MQMessageExt>::iterator itOrigin = msgs.begin(); itOrigin != msgs.end();) {
      bool remove = false;
      for (std::vector<MQMessageExt>::iterator itRetry = localRetryMsgs.begin(); itRetry != localRetryMsgs.end();
           itRetry++) {
        if (itRetry->getQueueOffset() == itOrigin->getQueueOffset()) {
          remove = true;
          break;
        }
      }
      if (remove) {
        itOrigin = msgs.erase(itOrigin);
      } else {
        itOrigin++;
      }
    }
  }
  // update offset
  int64 offset = request->removeMessage(msgs);
  if (offset >= 0) {
    m_pConsumer->updateConsumeOffset(request->m_messageQueue, offset);
  } else {
    LOG_WARN("Note: Get local offset for mq:%s failed, may be it is updated before. skip..",
             (request->m_messageQueue).toString().c_str());
  }
  if (!localRetryMsgs.empty()) {
    // submitConsumeRequest(request, localTryMsgs);
    LOG_INFO("Send [%d ]messages back to mq:%s failed, call reconsume again after 1s.", localRetryMsgs.size(),
             (request->m_messageQueue).toString().c_str());
    submitConsumeRequestLater(request, localRetryMsgs, 1000);
  }
}  // namespace rocketmq

void ConsumeMessageConcurrentlyService::resetRetryTopic(vector<MQMessageExt>& msgs) {
  string groupTopic = UtilAll::getRetryTopic(m_pConsumer->getGroupName());
  for (size_t i = 0; i < msgs.size(); i++) {
    MQMessageExt& msg = msgs[i];
    string retryTopic = msg.getProperty(MQMessage::PROPERTY_RETRY_TOPIC);
    if (!retryTopic.empty() && groupTopic.compare(msg.getTopic()) == 0) {
      msg.setTopic(retryTopic);
    }
  }
}

//<!***************************************************************************
}  // namespace rocketmq
