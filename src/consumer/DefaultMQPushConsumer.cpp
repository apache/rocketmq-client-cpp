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

#include "DefaultMQPushConsumer.h"
#include "CommunicationMode.h"
#include "ConsumeMsgService.h"
#include "ConsumerRunningInfo.h"
#include "FilterAPI.h"
#include "Logging.h"
#include "MQClientAPIImpl.h"
#include "MQClientFactory.h"
#include "OffsetStore.h"
#include "PullAPIWrapper.h"
#include "PullSysFlag.h"
#include "Rebalance.h"
#include "UtilAll.h"
#include "Validators.h"
#include "task_queue.h"

namespace rocketmq {

class AsyncPullCallback : public PullCallback {
 public:
  AsyncPullCallback(DefaultMQPushConsumer* pushConsumer, boost::weak_ptr<PullRequest> request)
      : m_callbackOwner(pushConsumer), m_pullRequest(request), m_bShutdown(false) {}

  virtual ~AsyncPullCallback() { m_callbackOwner = NULL; }

  virtual void onSuccess(MQMessageQueue& mq, PullResult& result, bool bProducePullRequest) {
    boost::shared_ptr<PullRequest> pullRequest = m_pullRequest.lock();
    if (!pullRequest) {
      LOG_WARN("Pull request for[%s] has been released", mq.toString().c_str());
      return;
    }

    if (m_bShutdown) {
      LOG_INFO("pullrequest for:%s in shutdown, return", (pullRequest->m_messageQueue).toString().c_str());
      return;
    }
    if (pullRequest->isDropped()) {
      LOG_INFO("Pull request for queue[%s] has been set as dropped. Will NOT pull this queue any more",
               pullRequest->m_messageQueue.toString().c_str());
      return;
    }
    switch (result.pullStatus) {
      case FOUND: {
        if (pullRequest->isDropped()) {
          LOG_INFO("[Dropped]Remove pullmsg event of mq:%s", (pullRequest->m_messageQueue).toString().c_str());
          break;
        }
        pullRequest->setNextOffset(result.nextBeginOffset);
        pullRequest->putMessage(result.msgFoundList);

        m_callbackOwner->getConsumerMsgService()->submitConsumeRequest(pullRequest, result.msgFoundList);

        if (bProducePullRequest) {
          m_callbackOwner->producePullMsgTask(pullRequest);
        } else {
          LOG_INFO("[bProducePullRequest = false]Stop pullmsg event of mq:%s",
                   (pullRequest->m_messageQueue).toString().c_str());
        }

        LOG_DEBUG("FOUND:%s with size:" SIZET_FMT ", nextBeginOffset:%lld",
                  (pullRequest->m_messageQueue).toString().c_str(), result.msgFoundList.size(), result.nextBeginOffset);

        break;
      }
      case NO_NEW_MSG: {
        if (pullRequest->isDropped()) {
          LOG_INFO("[Dropped]Remove pullmsg event of mq:%s", (pullRequest->m_messageQueue).toString().c_str());
          break;
        }
        pullRequest->setNextOffset(result.nextBeginOffset);

        vector<MQMessageExt> msgs;
        pullRequest->getMessage(msgs);
        if ((msgs.size() == 0) && (result.nextBeginOffset > 0)) {
          m_callbackOwner->updateConsumeOffset(pullRequest->m_messageQueue, result.nextBeginOffset);
        }
        if (bProducePullRequest) {
          m_callbackOwner->producePullMsgTask(pullRequest);
        } else {
          LOG_INFO("[bProducePullRequest = false]Stop pullmsg event of mq:%s",
                   (pullRequest->m_messageQueue).toString().c_str());
        }
        LOG_DEBUG("NO_NEW_MSG:%s,nextBeginOffset:%lld", pullRequest->m_messageQueue.toString().c_str(),
                  result.nextBeginOffset);
        break;
      }
      case NO_MATCHED_MSG: {
        if (pullRequest->isDropped()) {
          LOG_INFO("[Dropped]Remove pullmsg event of mq:%s", (pullRequest->m_messageQueue).toString().c_str());
          break;
        }
        pullRequest->setNextOffset(result.nextBeginOffset);

        vector<MQMessageExt> msgs;
        pullRequest->getMessage(msgs);
        if ((msgs.size() == 0) && (result.nextBeginOffset > 0)) {
          m_callbackOwner->updateConsumeOffset(pullRequest->m_messageQueue, result.nextBeginOffset);
        }
        if (bProducePullRequest) {
          m_callbackOwner->producePullMsgTask(pullRequest);
        } else {
          LOG_INFO("[bProducePullRequest = false]Stop pullmsg event of mq:%s",
                   (pullRequest->m_messageQueue).toString().c_str());
        }
        LOG_DEBUG("NO_MATCHED_MSG:%s,nextBeginOffset:%lld", pullRequest->m_messageQueue.toString().c_str(),
                  result.nextBeginOffset);
        break;
      }
      case OFFSET_ILLEGAL: {
        if (pullRequest->isDropped()) {
          LOG_INFO("[Dropped]Remove pullmsg event of mq:%s", (pullRequest->m_messageQueue).toString().c_str());
          break;
        }
        pullRequest->setNextOffset(result.nextBeginOffset);
        if (bProducePullRequest) {
          m_callbackOwner->producePullMsgTask(pullRequest);
        } else {
          LOG_INFO("[bProducePullRequest = false]Stop pullmsg event of mq:%s",
                   (pullRequest->m_messageQueue).toString().c_str());
        }

        LOG_DEBUG("OFFSET_ILLEGAL:%s,nextBeginOffset:%lld", pullRequest->m_messageQueue.toString().c_str(),
                  result.nextBeginOffset);
        break;
      }
      case BROKER_TIMEOUT: {
        if (pullRequest->isDropped()) {
          LOG_INFO("[Dropped]Remove pullmsg event of mq:%s", (pullRequest->m_messageQueue).toString().c_str());
          break;
        }
        LOG_ERROR("impossible BROKER_TIMEOUT Occurs");
        pullRequest->setNextOffset(result.nextBeginOffset);
        if (bProducePullRequest) {
          m_callbackOwner->producePullMsgTask(pullRequest);
        } else {
          LOG_INFO("[bProducePullRequest = false]Stop pullmsg event of mq:%s",
                   (pullRequest->m_messageQueue).toString().c_str());
        }
        break;
      }
    }
  }

  virtual void onException(MQException& e) {
    boost::shared_ptr<PullRequest> pullRequest = m_pullRequest.lock();
    if (!pullRequest) {
      LOG_WARN("Pull request has been released.");
      return;
    }
    std::string queueName = pullRequest->m_messageQueue.toString();
    if (m_bShutdown) {
      LOG_INFO("pullrequest for:%s in shutdown, return", queueName.c_str());
      return;
    }
    if (pullRequest->isDropped()) {
      LOG_INFO("[Dropped]Remove pullmsg event of mq:%s", queueName.c_str());
      return;
    }
    LOG_WARN("Pullrequest for:%s occurs exception, reproduce it after 1s.", queueName.c_str());
    m_callbackOwner->producePullMsgTaskLater(pullRequest, 1000);
  }

  void setShutdownStatus() { m_bShutdown = true; }

  const boost::weak_ptr<PullRequest>& getPullRequest() const { return m_pullRequest; }

  void setPullRequest(boost::weak_ptr<PullRequest>& pullRequest) { m_pullRequest = pullRequest; }

 private:
  DefaultMQPushConsumer* m_callbackOwner;
  boost::weak_ptr<PullRequest> m_pullRequest;
  bool m_bShutdown;
};

//<!***************************************************************************
static boost::mutex m_asyncCallbackLock;

DefaultMQPushConsumer::DefaultMQPushConsumer(const string& groupname)
    : m_consumeFromWhere(CONSUME_FROM_LAST_OFFSET),
      m_pOffsetStore(NULL),
      m_pRebalance(NULL),
      m_pPullAPIWrapper(NULL),
      m_consumerService(NULL),
      m_pMessageListener(NULL),
      m_consumeMessageBatchMaxSize(1),
      m_maxMsgCacheSize(1000),
      m_pullmsgQueue(NULL) {
  //<!set default group name;
  string gname = groupname.empty() ? DEFAULT_CONSUMER_GROUP : groupname;
  setGroupName(gname);
  m_asyncPull = true;
  m_asyncPullTimeout = 30 * 1000;
  setMessageModel(CLUSTERING);

  m_startTime = UtilAll::currentTimeMillis();
  m_consumeThreadCount = std::thread::hardware_concurrency();
  m_pullMsgThreadPoolNum = std::thread::hardware_concurrency();
  m_async_service_thread.reset(new boost::thread(boost::bind(&DefaultMQPushConsumer::boost_asio_work, this)));
}

void DefaultMQPushConsumer::boost_asio_work() {
  LOG_INFO("DefaultMQPushConsumer::boost asio async service runing");
  boost::asio::io_service::work work(m_async_ioService);  // avoid async io
  // service stops after
  // first timer timeout
  // callback
  m_async_ioService.run();
}

DefaultMQPushConsumer::~DefaultMQPushConsumer() {
  m_pMessageListener = NULL;
  if (m_pullmsgQueue != NULL) {
    deleteAndZero(m_pullmsgQueue);
  }
  if (m_pRebalance != NULL) {
    deleteAndZero(m_pRebalance);
  }
  if (m_pOffsetStore != NULL) {
    deleteAndZero(m_pOffsetStore);
  }
  if (m_pPullAPIWrapper != NULL) {
    deleteAndZero(m_pPullAPIWrapper);
  }
  if (m_consumerService != NULL) {
    deleteAndZero(m_consumerService);
  }
  PullMAP::iterator it = m_PullCallback.begin();
  for (; it != m_PullCallback.end(); ++it) {
    deleteAndZero(it->second);
  }
  m_PullCallback.clear();
  m_subTopics.clear();
}

void DefaultMQPushConsumer::sendMessageBack(MQMessageExt& msg, int delayLevel) {
  try {
    getFactory()->getMQClientAPIImpl()->consumerSendMessageBack(msg, getGroupName(), delayLevel, 3000,
                                                                getSessionCredentials());
  } catch (MQException& e) {
    LOG_ERROR(e.what());
  }
}

void DefaultMQPushConsumer::fetchSubscribeMessageQueues(const string& topic, vector<MQMessageQueue>& mqs) {
  mqs.clear();
  try {
    getFactory()->fetchSubscribeMessageQueues(topic, mqs, getSessionCredentials());
  } catch (MQException& e) {
    LOG_ERROR(e.what());
  }
}

void DefaultMQPushConsumer::doRebalance() {
  if (isServiceStateOk()) {
    try {
      m_pRebalance->doRebalance();
    } catch (MQException& e) {
      LOG_ERROR(e.what());
    }
  }
}

void DefaultMQPushConsumer::persistConsumerOffset() {
  if (isServiceStateOk()) {
    m_pRebalance->persistConsumerOffset();
  }
}

void DefaultMQPushConsumer::persistConsumerOffsetByResetOffset() {
  if (isServiceStateOk()) {
    m_pRebalance->persistConsumerOffsetByResetOffset();
  }
}

void DefaultMQPushConsumer::start() {
#ifndef WIN32
  /* Ignore the SIGPIPE */
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, 0);
#endif
  switch (m_serviceState) {
    case CREATE_JUST: {
      m_serviceState = START_FAILED;
      MQClient::start();
      LOG_INFO("DefaultMQPushConsumer:%s start", m_GroupName.c_str());

      //<!data;
      checkConfig();

      //<!create rebalance;
      m_pRebalance = new RebalancePush(this, getFactory());

      string groupname = getGroupName();
      m_pPullAPIWrapper = new PullAPIWrapper(getFactory(), groupname);

      if (m_pMessageListener) {
        if (m_pMessageListener->getMessageListenerType() == messageListenerOrderly) {
          LOG_INFO("start orderly consume service:%s", getGroupName().c_str());
          m_consumerService = new ConsumeMessageOrderlyService(this, m_consumeThreadCount, m_pMessageListener);
        } else  // for backward compatible, defaultly and concurrently listeners
                // are allocating ConsumeMessageConcurrentlyService
        {
          LOG_INFO("start concurrently consume service:%s", getGroupName().c_str());
          m_consumerService = new ConsumeMessageConcurrentlyService(this, m_consumeThreadCount, m_pMessageListener);
        }
      }

      m_pullmsgQueue = new TaskQueue(m_pullMsgThreadPoolNum);
      m_pullmsgThread.reset(
          new boost::thread(boost::bind(&DefaultMQPushConsumer::runPullMsgQueue, this, m_pullmsgQueue)));

      copySubscription();

      //<! registe;
      bool registerOK = getFactory()->registerConsumer(this);
      if (!registerOK) {
        m_serviceState = CREATE_JUST;
        THROW_MQEXCEPTION(
            MQClientException,
            "The cousumer group[" + getGroupName() + "] has been created before, specify another name please.", -1);
      }

      //<!msg model;
      switch (getMessageModel()) {
        case BROADCASTING:
          m_pOffsetStore = new LocalFileOffsetStore(groupname, getFactory());
          break;
        case CLUSTERING:
          m_pOffsetStore = new RemoteBrokerOffsetStore(groupname, getFactory());
          break;
      }
      bool bStartFailed = false;
      string errorMsg;
      try {
        m_pOffsetStore->load();
      } catch (MQClientException& e) {
        bStartFailed = true;
        errorMsg = std::string(e.what());
      }
      m_consumerService->start();

      getFactory()->start();

      updateTopicSubscribeInfoWhenSubscriptionChanged();
      getFactory()->sendHeartbeatToAllBroker();

      m_serviceState = RUNNING;
      if (bStartFailed) {
        shutdown();
        THROW_MQEXCEPTION(MQClientException, errorMsg, -1);
      }
      break;
    }
    case RUNNING:
    case START_FAILED:
    case SHUTDOWN_ALREADY:
      break;
    default:
      break;
  }

  getFactory()->rebalanceImmediately();
}

void DefaultMQPushConsumer::shutdown() {
  switch (m_serviceState) {
    case RUNNING: {
      LOG_INFO("DefaultMQPushConsumer shutdown");
      m_async_ioService.stop();
      m_async_service_thread->interrupt();
      m_async_service_thread->join();
      m_pullmsgQueue->close();
      m_pullmsgThread->interrupt();
      m_pullmsgThread->join();
      m_consumerService->shutdown();
      persistConsumerOffset();
      shutdownAsyncPullCallBack();  // delete aync pullMsg resources
      getFactory()->unregisterConsumer(this);
      getFactory()->shutdown();
      m_serviceState = SHUTDOWN_ALREADY;
      break;
    }
    case CREATE_JUST:
    case SHUTDOWN_ALREADY:
      break;
    default:
      break;
  }
}

void DefaultMQPushConsumer::registerMessageListener(MQMessageListener* pMessageListener) {
  if (NULL != pMessageListener) {
    m_pMessageListener = pMessageListener;
  }
}

MessageListenerType DefaultMQPushConsumer::getMessageListenerType() {
  if (NULL != m_pMessageListener) {
    return m_pMessageListener->getMessageListenerType();
  }
  return messageListenerDefaultly;
}

ConsumeMsgService* DefaultMQPushConsumer::getConsumerMsgService() const {
  return m_consumerService;
}

OffsetStore* DefaultMQPushConsumer::getOffsetStore() const {
  return m_pOffsetStore;
}

Rebalance* DefaultMQPushConsumer::getRebalance() const {
  return m_pRebalance;
}

void DefaultMQPushConsumer::subscribe(const string& topic, const string& subExpression) {
  m_subTopics[topic] = subExpression;
}

void DefaultMQPushConsumer::checkConfig() {
  string groupname = getGroupName();
  // check consumerGroup
  Validators::checkGroup(groupname);

  // consumerGroup
  if (!groupname.compare(DEFAULT_CONSUMER_GROUP)) {
    THROW_MQEXCEPTION(MQClientException, "consumerGroup can not equal DEFAULT_CONSUMER", -1);
  }

  if (getMessageModel() != BROADCASTING && getMessageModel() != CLUSTERING) {
    THROW_MQEXCEPTION(MQClientException, "messageModel is valid ", -1);
  }

  if (m_pMessageListener == NULL) {
    THROW_MQEXCEPTION(MQClientException, "messageListener is null ", -1);
  }
}

void DefaultMQPushConsumer::copySubscription() {
  map<string, string>::iterator it = m_subTopics.begin();
  for (; it != m_subTopics.end(); ++it) {
    LOG_INFO("buildSubscriptionData,:%s,%s", it->first.c_str(), it->second.c_str());
    unique_ptr<SubscriptionData> pSData(FilterAPI::buildSubscriptionData(it->first, it->second));

    m_pRebalance->setSubscriptionData(it->first, pSData.release());
  }

  switch (getMessageModel()) {
    case BROADCASTING:
      break;
    case CLUSTERING: {
      string retryTopic = UtilAll::getRetryTopic(getGroupName());

      //<!this sub;
      unique_ptr<SubscriptionData> pSData(FilterAPI::buildSubscriptionData(retryTopic, SUB_ALL));

      m_pRebalance->setSubscriptionData(retryTopic, pSData.release());
      break;
    }
    default:
      break;
  }
}

void DefaultMQPushConsumer::updateTopicSubscribeInfo(const string& topic, vector<MQMessageQueue>& info) {
  m_pRebalance->setTopicSubscribeInfo(topic, info);
}

void DefaultMQPushConsumer::updateTopicSubscribeInfoWhenSubscriptionChanged() {
  map<string, SubscriptionData*>& subTable = m_pRebalance->getSubscriptionInner();
  map<string, SubscriptionData*>::iterator it = subTable.begin();
  for (; it != subTable.end(); ++it) {
    bool btopic = getFactory()->updateTopicRouteInfoFromNameServer(it->first, getSessionCredentials());
    if (btopic == false) {
      LOG_WARN("The topic:[%s] not exist", it->first.c_str());
    }
  }
}

ConsumeType DefaultMQPushConsumer::getConsumeType() {
  return CONSUME_PASSIVELY;
}

ConsumeFromWhere DefaultMQPushConsumer::getConsumeFromWhere() {
  return m_consumeFromWhere;
}

void DefaultMQPushConsumer::setConsumeFromWhere(ConsumeFromWhere consumeFromWhere) {
  m_consumeFromWhere = consumeFromWhere;
}

void DefaultMQPushConsumer::getSubscriptions(vector<SubscriptionData>& result) {
  map<string, SubscriptionData*>& subTable = m_pRebalance->getSubscriptionInner();
  map<string, SubscriptionData*>::iterator it = subTable.begin();
  for (; it != subTable.end(); ++it) {
    result.push_back(*(it->second));
  }
}

void DefaultMQPushConsumer::updateConsumeOffset(const MQMessageQueue& mq, int64 offset) {
  if (offset >= 0) {
    m_pOffsetStore->updateOffset(mq, offset);
  } else {
    LOG_ERROR("updateConsumeOffset of mq:%s error", mq.toString().c_str());
  }
}

void DefaultMQPushConsumer::removeConsumeOffset(const MQMessageQueue& mq) {
  m_pOffsetStore->removeOffset(mq);
}

void DefaultMQPushConsumer::static_triggerNextPullRequest(void* context,
                                                          boost::asio::deadline_timer* t,
                                                          boost::weak_ptr<PullRequest> pullRequest) {
  if (pullRequest.expired()) {
    LOG_WARN("Pull request has been released before.");
    return;
  }
  DefaultMQPushConsumer* pDefaultMQPushConsumer = (DefaultMQPushConsumer*)context;
  if (pDefaultMQPushConsumer) {
    pDefaultMQPushConsumer->triggerNextPullRequest(t, pullRequest);
  }
}

void DefaultMQPushConsumer::triggerNextPullRequest(boost::asio::deadline_timer* t,
                                                   boost::weak_ptr<PullRequest> pullRequest) {
  // delete first to avoild memleak
  deleteAndZero(t);
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_WARN("Pull request has been released before.");
    return;
  }
  producePullMsgTask(request);
}

bool DefaultMQPushConsumer::producePullMsgTaskLater(boost::weak_ptr<PullRequest> pullRequest, int millis) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_INFO("Pull request is invalid. Maybe it is dropped before.");
    return false;
  }
  if (request->isDropped()) {
    LOG_INFO("[Dropped]Remove pullmsg event of mq:%s", request->m_messageQueue.toString().c_str());
    return false;
  }
  boost::asio::deadline_timer* t =
      new boost::asio::deadline_timer(m_async_ioService, boost::posix_time::milliseconds(millis));
  t->async_wait(boost::bind(&(DefaultMQPushConsumer::static_triggerNextPullRequest), this, t, request));
  LOG_INFO("Produce Pull request [%s] Later and Sleep [%d]ms.", (request->m_messageQueue).toString().c_str(), millis);
  return true;
}

bool DefaultMQPushConsumer::producePullMsgTask(boost::weak_ptr<PullRequest> pullRequest) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_WARN("Pull request has been released.");
    return false;
  }
  if (request->isDropped()) {
    LOG_INFO("[Dropped]Remove pullmsg event of mq:%s", request->m_messageQueue.toString().c_str());
    return false;
  }
  if (m_pullmsgQueue->bTaskQueueStatusOK() && isServiceStateOk()) {
    if (m_asyncPull) {
      m_pullmsgQueue->produce(TaskBinder::gen(&DefaultMQPushConsumer::pullMessageAsync, this, request));
    } else {
      m_pullmsgQueue->produce(TaskBinder::gen(&DefaultMQPushConsumer::pullMessage, this, request));
    }
  } else {
    LOG_WARN("produce PullRequest of mq:%s failed", request->m_messageQueue.toString().c_str());
    return false;
  }
  return true;
}

void DefaultMQPushConsumer::runPullMsgQueue(TaskQueue* pTaskQueue) {
  pTaskQueue->run();
}

void DefaultMQPushConsumer::pullMessage(boost::weak_ptr<PullRequest> pullRequest) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_ERROR("Pull request is released, return");
    return;
  }
  if (request->isDropped()) {
    LOG_WARN("Pull request is set drop with mq:%s, return", (request->m_messageQueue).toString().c_str());
    // request->removePullMsgEvent();
    return;
  }

  MQMessageQueue& messageQueue = request->m_messageQueue;
  if (m_consumerService->getConsumeMsgSerivceListenerType() == messageListenerOrderly) {
    if (!request->isLocked() || request->isLockExpired()) {
      if (!m_pRebalance->lock(messageQueue)) {
        request->setLastPullTimestamp(UtilAll::currentTimeMillis());
        producePullMsgTaskLater(request, 1000);
        return;
      }
    }
  }

  if (request->getCacheMsgCount() > m_maxMsgCacheSize) {
    LOG_INFO("Sync Pull request for %s has Cached with %d Messages and The Max size is %d, Sleep 1s.",
             (request->m_messageQueue).toString().c_str(), request->getCacheMsgCount(), m_maxMsgCacheSize);
    request->setLastPullTimestamp(UtilAll::currentTimeMillis());
    // Retry 1s,
    producePullMsgTaskLater(request, 1000);
    return;
  }

  bool commitOffsetEnable = false;
  int64 commitOffsetValue = 0;
  if (CLUSTERING == getMessageModel()) {
    commitOffsetValue = m_pOffsetStore->readOffset(messageQueue, READ_FROM_MEMORY, getSessionCredentials());
    if (commitOffsetValue > 0) {
      commitOffsetEnable = true;
    }
  }

  string subExpression;
  SubscriptionData* pSdata = m_pRebalance->getSubscriptionData(messageQueue.getTopic());
  if (pSdata == NULL) {
    LOG_INFO("Can not get SubscriptionData of Pull request for [%s], Sleep 1s.",
             (request->m_messageQueue).toString().c_str());
    producePullMsgTaskLater(request, 1000);
    return;
  }
  subExpression = pSdata->getSubString();

  int sysFlag = PullSysFlag::buildSysFlag(commitOffsetEnable,      // commitOffset
                                          false,                   // suspend
                                          !subExpression.empty(),  // subscription
                                          false);                  // class filter
  if (request->isDropped()) {
    LOG_WARN("Pull request is set as dropped with mq:%s, return", (request->m_messageQueue).toString().c_str());
    return;
  }
  try {
    request->setLastPullTimestamp(UtilAll::currentTimeMillis());
    unique_ptr<PullResult> result(m_pPullAPIWrapper->pullKernelImpl(messageQueue,              // 1
                                                                    subExpression,             // 2
                                                                    pSdata->getSubVersion(),   // 3
                                                                    request->getNextOffset(),  // 4
                                                                    32,                        // 5
                                                                    sysFlag,                   // 6
                                                                    commitOffsetValue,         // 7
                                                                    1000 * 15,                 // 8
                                                                    1000 * 30,                 // 9
                                                                    ComMode_SYNC,              // 10
                                                                    NULL, getSessionCredentials()));

    PullResult pullResult = m_pPullAPIWrapper->processPullResult(messageQueue, result.get(), pSdata);
    switch (pullResult.pullStatus) {
      case FOUND: {
        if (request->isDropped()) {
          LOG_INFO("Get pull result but the queue has been marked as dropped. Queue: %s",
                   messageQueue.toString().c_str());
          break;
        }
        // and this request is dropped, and then received pulled msgs.
        request->setNextOffset(pullResult.nextBeginOffset);
        request->putMessage(pullResult.msgFoundList);

        m_consumerService->submitConsumeRequest(request, pullResult.msgFoundList);
        producePullMsgTask(request);

        LOG_DEBUG("FOUND:%s with size:" SIZET_FMT ",nextBeginOffset:%lld", messageQueue.toString().c_str(),
                  pullResult.msgFoundList.size(), pullResult.nextBeginOffset);

        break;
      }
      case NO_NEW_MSG: {
        if (request->isDropped()) {
          LOG_INFO("Get pull result but the queue has been marked as dropped. Queue: %s",
                   messageQueue.toString().c_str());
          break;
        }
        request->setNextOffset(pullResult.nextBeginOffset);
        vector<MQMessageExt> msgs;
        request->getMessage(msgs);
        if ((msgs.size() == 0) && (pullResult.nextBeginOffset > 0)) {
          updateConsumeOffset(messageQueue, pullResult.nextBeginOffset);
        }
        producePullMsgTask(request);
        LOG_DEBUG("NO_NEW_MSG:%s,nextBeginOffset:%lld", messageQueue.toString().c_str(), pullResult.nextBeginOffset);
        break;
      }
      case NO_MATCHED_MSG: {
        if (request->isDropped()) {
          LOG_INFO("Get pull result but the queue has been marked as dropped. Queue: %s",
                   messageQueue.toString().c_str());
          break;
        }
        request->setNextOffset(pullResult.nextBeginOffset);
        vector<MQMessageExt> msgs;
        request->getMessage(msgs);
        if ((msgs.size() == 0) && (pullResult.nextBeginOffset > 0)) {
          updateConsumeOffset(messageQueue, pullResult.nextBeginOffset);
        }
        producePullMsgTask(request);

        LOG_DEBUG("NO_MATCHED_MSG:%s,nextBeginOffset:%lld", messageQueue.toString().c_str(),
                  pullResult.nextBeginOffset);
        break;
      }
      case OFFSET_ILLEGAL: {
        if (request->isDropped()) {
          LOG_INFO("Get pull result but the queue has been marked as dropped. Queue: %s",
                   messageQueue.toString().c_str());
          break;
        }
        request->setNextOffset(pullResult.nextBeginOffset);
        producePullMsgTask(request);

        LOG_DEBUG("OFFSET_ILLEGAL:%s,nextBeginOffset:%lld", messageQueue.toString().c_str(),
                  pullResult.nextBeginOffset);
        break;
      }
      case BROKER_TIMEOUT: {  // as BROKER_TIMEOUT is defined by client, broker
        // will not returns this status, so this case
        // could not be entered.
        LOG_ERROR("impossible BROKER_TIMEOUT Occurs");
        request->setNextOffset(pullResult.nextBeginOffset);
        producePullMsgTask(request);
        break;
      }
    }
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    LOG_WARN("Pull %s occur exception, restart 1s  later.", messageQueue.toString().c_str());
    producePullMsgTaskLater(request, 1000);
  }
}

AsyncPullCallback* DefaultMQPushConsumer::getAsyncPullCallBack(boost::weak_ptr<PullRequest> pullRequest,
                                                               MQMessageQueue msgQueue) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    return NULL;
  }
  boost::lock_guard<boost::mutex> lock(m_asyncCallbackLock);
  if (m_asyncPull && request) {
    PullMAP::iterator it = m_PullCallback.find(msgQueue);
    if (it == m_PullCallback.end()) {
      LOG_INFO("new pull callback for mq:%s", msgQueue.toString().c_str());
      m_PullCallback[msgQueue] = new AsyncPullCallback(this, request);
    }
    AsyncPullCallback* asyncPullCallback = m_PullCallback[msgQueue];
    if (asyncPullCallback && asyncPullCallback->getPullRequest().expired()) {
      asyncPullCallback->setPullRequest(pullRequest);
    }
    return asyncPullCallback;
  }

  return NULL;
}

void DefaultMQPushConsumer::shutdownAsyncPullCallBack() {
  boost::lock_guard<boost::mutex> lock(m_asyncCallbackLock);
  if (m_asyncPull) {
    PullMAP::iterator it = m_PullCallback.begin();
    for (; it != m_PullCallback.end(); ++it) {
      if (it->second) {
        it->second->setShutdownStatus();
      } else {
        LOG_ERROR("could not find asyncPullCallback for:%s", it->first.toString().c_str());
      }
    }
  }
}

void DefaultMQPushConsumer::pullMessageAsync(boost::weak_ptr<PullRequest> pullRequest) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_ERROR("Pull request is released, return");
    return;
  }
  if (request->isDropped()) {
    LOG_WARN("Pull request is set drop with mq:%s, return", (request->m_messageQueue).toString().c_str());
    return;
  }
  MQMessageQueue& messageQueue = request->m_messageQueue;
  if (m_consumerService->getConsumeMsgSerivceListenerType() == messageListenerOrderly) {
    if (!request->isLocked() || request->isLockExpired()) {
      if (!m_pRebalance->lock(messageQueue)) {
        request->setLastPullTimestamp(UtilAll::currentTimeMillis());
        // Retry later.
        producePullMsgTaskLater(request, 1000);
        return;
      }
    }
  }

  if (request->getCacheMsgCount() > m_maxMsgCacheSize) {
    LOG_INFO("Pull request for [%s] has Cached with %d Messages and The Max size is %d, Sleep 3s.",
             (request->m_messageQueue).toString().c_str(), request->getCacheMsgCount(), m_maxMsgCacheSize);
    request->setLastPullTimestamp(UtilAll::currentTimeMillis());
    // Retry 3s,
    producePullMsgTaskLater(request, 3000);
    return;
  }

  bool commitOffsetEnable = false;
  int64 commitOffsetValue = 0;
  if (CLUSTERING == getMessageModel()) {
    commitOffsetValue = m_pOffsetStore->readOffset(messageQueue, READ_FROM_MEMORY, getSessionCredentials());
    if (commitOffsetValue > 0) {
      commitOffsetEnable = true;
    }
  }

  string subExpression;
  SubscriptionData* pSdata = (m_pRebalance->getSubscriptionData(messageQueue.getTopic()));
  if (pSdata == NULL) {
    LOG_INFO("Can not get SubscriptionData of Pull request for [%s], Sleep 1s.",
             (request->m_messageQueue).toString().c_str());
    // Subscribe data error, retry later.
    producePullMsgTaskLater(request, 1000);
    return;
  }
  subExpression = pSdata->getSubString();

  int sysFlag = PullSysFlag::buildSysFlag(commitOffsetEnable,      // commitOffset
                                          true,                    // suspend
                                          !subExpression.empty(),  // subscription
                                          false);                  // class filter

  AsyncArg arg;
  arg.mq = messageQueue;
  arg.subData = *pSdata;
  arg.pPullWrapper = m_pPullAPIWrapper;
  if (request->isDropped()) {
    LOG_WARN("Pull request is set as dropped with mq:%s, return", request->m_messageQueue.toString().c_str());
    return;
  }
  try {
    request->setLastPullTimestamp(UtilAll::currentTimeMillis());
    AsyncPullCallback* pullCallback = getAsyncPullCallBack(request, messageQueue);
    if (pullCallback == NULL) {
      LOG_WARN("Can not get pull callback for:%s, Maybe this pull request has been released.",
               request->m_messageQueue.toString().c_str());
      return;
    }
    m_pPullAPIWrapper->pullKernelImpl(messageQueue,              // 1
                                      subExpression,             // 2
                                      pSdata->getSubVersion(),   // 3
                                      request->getNextOffset(),  // 4
                                      32,                        // 5
                                      sysFlag,                   // 6
                                      commitOffsetValue,         // 7
                                      1000 * 15,                 // 8
                                      m_asyncPullTimeout,        // 9
                                      ComMode_ASYNC,             // 10
                                      pullCallback,              // 11
                                      getSessionCredentials(),   // 12
                                      &arg);                     // 13
  } catch (MQException& e) {
    LOG_ERROR(e.what());
    if (request->isDropped()) {
      LOG_WARN("Pull request is set as dropped with mq:%s, return", (request->m_messageQueue).toString().c_str());
      return;
    }
    LOG_INFO("Pull %s occur exception, restart 1s  later.", (request->m_messageQueue).toString().c_str());
    producePullMsgTaskLater(request, 1000);
  }
}

void DefaultMQPushConsumer::setAsyncPull(bool asyncFlag) {
  if (asyncFlag) {
    LOG_INFO("set pushConsumer:%s to async default pull mode", getGroupName().c_str());
  } else {
    LOG_INFO("set pushConsumer:%s to sync pull mode", getGroupName().c_str());
  }
  m_asyncPull = asyncFlag;
}

void DefaultMQPushConsumer::setConsumeThreadCount(int threadCount) {
  if (threadCount > 0) {
    m_consumeThreadCount = threadCount;
  } else {
    LOG_ERROR("setConsumeThreadCount with invalid value");
  }
}

int DefaultMQPushConsumer::getConsumeThreadCount() const {
  return m_consumeThreadCount;
}

void DefaultMQPushConsumer::setPullMsgThreadPoolCount(int threadCount) {
  m_pullMsgThreadPoolNum = threadCount;
}

int DefaultMQPushConsumer::getPullMsgThreadPoolCount() const {
  return m_pullMsgThreadPoolNum;
}

int DefaultMQPushConsumer::getConsumeMessageBatchMaxSize() const {
  return m_consumeMessageBatchMaxSize;
}

void DefaultMQPushConsumer::setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize) {
  if (consumeMessageBatchMaxSize >= 1)
    m_consumeMessageBatchMaxSize = consumeMessageBatchMaxSize;
}

void DefaultMQPushConsumer::setMaxCacheMsgSizePerQueue(int maxCacheSize) {
  if (maxCacheSize > 0 && maxCacheSize < 65535) {
    LOG_INFO("set maxCacheSize to:%d for consumer:%s", maxCacheSize, getGroupName().c_str());
    m_maxMsgCacheSize = maxCacheSize;
  }
}

int DefaultMQPushConsumer::getMaxCacheMsgSizePerQueue() const {
  return m_maxMsgCacheSize;
}

ConsumerRunningInfo* DefaultMQPushConsumer::getConsumerRunningInfo() {
  auto* info = new ConsumerRunningInfo();
  if (m_consumerService->getConsumeMsgSerivceListenerType() == messageListenerOrderly) {
    info->setProperty(ConsumerRunningInfo::PROP_CONSUME_ORDERLY, "true");
  } else {
    info->setProperty(ConsumerRunningInfo::PROP_CONSUME_ORDERLY, "false");
  }
  info->setProperty(ConsumerRunningInfo::PROP_THREADPOOL_CORE_SIZE, UtilAll::to_string(m_consumeThreadCount));
  info->setProperty(ConsumerRunningInfo::PROP_CONSUMER_START_TIMESTAMP, UtilAll::to_string(m_startTime));

  std::vector<SubscriptionData> result;
  getSubscriptions(result);
  info->setSubscriptionSet(result);

  std::map<MQMessageQueue, boost::shared_ptr<PullRequest>> requestTable = m_pRebalance->getPullRequestTable();

  for (const auto& it : requestTable) {
    if (!it.second->isDropped()) {
      MessageQueue queue((it.first).getTopic(), (it.first).getBrokerName(), (it.first).getQueueId());
      ProcessQueueInfo processQueue;
      processQueue.cachedMsgMinOffset = it.second->getCacheMinOffset();
      processQueue.cachedMsgMaxOffset = it.second->getCacheMaxOffset();
      processQueue.cachedMsgCount = it.second->getCacheMsgCount();
      processQueue.setCommitOffset(
          m_pOffsetStore->readOffset(it.first, MEMORY_FIRST_THEN_STORE, getSessionCredentials()));
      processQueue.setDroped(it.second->isDropped());
      processQueue.setLocked(it.second->isLocked());
      processQueue.lastLockTimestamp = it.second->getLastLockTimestamp();
      processQueue.lastPullTimestamp = it.second->getLastPullTimestamp();
      processQueue.lastConsumeTimestamp = it.second->getLastConsumeTimestamp();
      info->setMqTable(queue, processQueue);
    }
  }

  return info;
}

//<!************************************************************************
}  // namespace rocketmq
