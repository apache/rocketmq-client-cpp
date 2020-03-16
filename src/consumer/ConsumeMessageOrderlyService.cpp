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

#include <MessageAccessor.h>
#include "ConsumeMsgService.h"
#include "DefaultMQPushConsumer.h"
#include "Logging.h"
#include "Rebalance.h"
#include "UtilAll.h"

namespace rocketmq {

//<!***************************************************************************
ConsumeMessageOrderlyService::ConsumeMessageOrderlyService(MQConsumer* consumer,
                                                           int threadCount,
                                                           MQMessageListener* msgListener)
    : m_pConsumer(consumer),
      m_shutdownInprogress(false),
      m_pMessageListener(msgListener),
      m_MaxTimeConsumeContinuously(60 * 1000),
      m_ioServiceWork(m_ioService),
      m_async_service_thread(NULL) {
#if !defined(WIN32) && !defined(__APPLE__)
  string taskName = UtilAll::getProcessName();
  prctl(PR_SET_NAME, "oderlyConsumeTP", 0, 0, 0);
#endif
  for (int i = 0; i != threadCount; ++i) {
    m_threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &m_ioService));
  }
#if !defined(WIN32) && !defined(__APPLE__)
  prctl(PR_SET_NAME, taskName.c_str(), 0, 0, 0);
#endif
}

void ConsumeMessageOrderlyService::boost_asio_work() {
  LOG_INFO("ConsumeMessageOrderlyService::boost asio async service runing");
  boost::asio::io_service::work work(m_async_ioService);  // avoid async io
                                                          // service stops after
                                                          // first timer timeout
                                                          // callback
  boost::system::error_code ec;
  boost::asio::deadline_timer t(m_async_ioService, boost::posix_time::milliseconds(PullRequest::RebalanceLockInterval));
  t.async_wait(boost::bind(&ConsumeMessageOrderlyService::lockMQPeriodically, this, ec, &t));

  m_async_ioService.run();
}

ConsumeMessageOrderlyService::~ConsumeMessageOrderlyService(void) {
  m_pConsumer = NULL;
  m_pMessageListener = NULL;
}

void ConsumeMessageOrderlyService::start() {
  m_async_service_thread.reset(new boost::thread(boost::bind(&ConsumeMessageOrderlyService::boost_asio_work, this)));
}

void ConsumeMessageOrderlyService::shutdown() {
  stopThreadPool();
  unlockAllMQ();
}

void ConsumeMessageOrderlyService::lockMQPeriodically(boost::system::error_code& ec, boost::asio::deadline_timer* t) {
  m_pConsumer->getRebalance()->lockAll();

  boost::system::error_code e;
  t->expires_at(t->expires_at() + boost::posix_time::milliseconds(PullRequest::RebalanceLockInterval), e);
  t->async_wait(boost::bind(&ConsumeMessageOrderlyService::lockMQPeriodically, this, ec, t));
}

void ConsumeMessageOrderlyService::unlockAllMQ() {
  m_pConsumer->getRebalance()->unlockAll(false);
}

bool ConsumeMessageOrderlyService::lockOneMQ(const MQMessageQueue& mq) {
  return m_pConsumer->getRebalance()->lock(mq);
}

void ConsumeMessageOrderlyService::stopThreadPool() {
  m_shutdownInprogress = true;
  m_ioService.stop();
  m_async_ioService.stop();
  m_async_service_thread->interrupt();
  m_async_service_thread->join();
  m_threadpool.join_all();
}

MessageListenerType ConsumeMessageOrderlyService::getConsumeMsgSerivceListenerType() {
  return m_pMessageListener->getMessageListenerType();
}

void ConsumeMessageOrderlyService::submitConsumeRequest(boost::weak_ptr<PullRequest> pullRequest,
                                                        vector<MQMessageExt>& msgs) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_WARN("Pull request has been released");
    return;
  }
  m_ioService.post(boost::bind(&ConsumeMessageOrderlyService::ConsumeRequest, this, request));
}

void ConsumeMessageOrderlyService::static_submitConsumeRequestLater(void* context,
                                                                    boost::weak_ptr<PullRequest> pullRequest,
                                                                    bool tryLockMQ,
                                                                    boost::asio::deadline_timer* t) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_WARN("Pull request has been released");
    return;
  }
  LOG_INFO("submit consumeRequest later for mq:%s", request->m_messageQueue.toString().c_str());
  vector<MQMessageExt> msgs;
  ConsumeMessageOrderlyService* orderlyService = (ConsumeMessageOrderlyService*)context;
  orderlyService->submitConsumeRequest(request, msgs);
  if (tryLockMQ) {
    orderlyService->lockOneMQ(request->m_messageQueue);
  }
  if (t)
    deleteAndZero(t);
}

void ConsumeMessageOrderlyService::ConsumeRequest(boost::weak_ptr<PullRequest> pullRequest) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_WARN("Pull request has been released");
    return;
  }
  bool bGetMutex = false;
  boost::unique_lock<boost::timed_mutex> lock(request->getPullRequestCriticalSection(), boost::try_to_lock);
  if (!lock.owns_lock()) {
    if (!lock.timed_lock(boost::get_system_time() + boost::posix_time::seconds(1))) {
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
  if (!request || request->isDropped()) {
    LOG_WARN("the pull result is NULL or Had been dropped");
    request->clearAllMsgs();  // add clear operation to avoid bad state when
                              // dropped pullRequest returns normal
    return;
  }

  if (m_pMessageListener) {
    if ((request->isLocked() && !request->isLockExpired()) || m_pConsumer->getMessageModel() == BROADCASTING) {
      // DefaultMQPushConsumer* pConsumer = (DefaultMQPushConsumer*)m_pConsumer;
      uint64_t beginTime = UtilAll::currentTimeMillis();
      bool continueConsume = true;
      while (continueConsume) {
        if ((UtilAll::currentTimeMillis() - beginTime) > m_MaxTimeConsumeContinuously) {
          LOG_INFO("Continuely consume %s more than 60s, consume it 1s later",
                   request->m_messageQueue.toString().c_str());
          tryLockLaterAndReconsumeDelay(request, false, 1000);
          break;
        }
        vector<MQMessageExt> msgs;
        // request->takeMessages(msgs, pConsumer->getConsumeMessageBatchMaxSize());
        request->takeMessages(msgs, 1);
        if (!msgs.empty()) {
          request->setLastConsumeTimestamp(UtilAll::currentTimeMillis());
          if (m_pConsumer->isUseNameSpaceMode()) {
            MessageAccessor::withoutNameSpace(msgs, m_pConsumer->getNameSpace());
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
          ConsumeStatus consumeStatus = m_pMessageListener->consumeMessage(msgs);
          if (consumeStatus == RECONSUME_LATER) {
            if (pConsumer) {
              consumeMessageContext.setMsgIndex(0);
              consumeMessageContext.setStatus("RECONSUME_LATER");
              consumeMessageContext.setSuccess(false);
              pConsumer->executeConsumeMessageHookAfter(&consumeMessageContext);
            }
            if (msgs[0].getReconsumeTimes() <= 15) {
              msgs[0].setReconsumeTimes(msgs[0].getReconsumeTimes() + 1);
              request->makeMessageToCosumeAgain(msgs);
              continueConsume = false;
              tryLockLaterAndReconsumeDelay(request, false, 1000);
            } else {
              // need change to reconsumer delay level and print log.
              LOG_INFO("Local Consume failed [%d] times, change [%s] delay to 5s.", msgs[0].getReconsumeTimes(),
                       msgs[0].getMsgId().c_str());
              msgs[0].setReconsumeTimes(msgs[0].getReconsumeTimes() + 1);
              continueConsume = false;
              request->makeMessageToCosumeAgain(msgs);
              tryLockLaterAndReconsumeDelay(request, false, 5000);
            }
          } else {
            if (pConsumer) {
              consumeMessageContext.setMsgIndex(0);
              consumeMessageContext.setStatus("CONSUME_SUCCESS");
              consumeMessageContext.setSuccess(true);
              pConsumer->executeConsumeMessageHookAfter(&consumeMessageContext);
            }
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
      tryLockLaterAndReconsumeDelay(request, true, 1000);
    }
  }
}
void ConsumeMessageOrderlyService::tryLockLaterAndReconsumeDelay(boost::weak_ptr<PullRequest> pullRequest,
                                                                 bool tryLockMQ,
                                                                 int millisDelay) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_WARN("Pull request has been released");
    return;
  }
  int retryTimer = millisDelay;
  if (millisDelay >= 30000 || millisDelay <= 1000) {
    retryTimer = 1000;
  }
  boost::asio::deadline_timer* t =
      new boost::asio::deadline_timer(m_async_ioService, boost::posix_time::milliseconds(retryTimer));
  t->async_wait(
      boost::bind(&ConsumeMessageOrderlyService::static_submitConsumeRequestLater, this, request, tryLockMQ, t));
}

}  // namespace rocketmq
