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

#include "DefaultMQPushConsumerImpl.h"
#include "CommunicationMode.h"
#include "ConsumeMessageHookImpl.h"
#include "ConsumeMsgService.h"
#include "ConsumerRunningInfo.h"
#include "FilterAPI.h"
#include "Logging.h"
#include "MQClientAPIImpl.h"
#include "MQClientFactory.h"
#include "NameSpaceUtil.h"
#include "OffsetStore.h"
#include "PullAPIWrapper.h"
#include "PullSysFlag.h"
#include "Rebalance.h"
#include "StatsServerManager.h"
#include "UtilAll.h"
#include "Validators.h"
#include "task_queue.h"

namespace rocketmq {

class AsyncPullCallback : public PullCallback {
 public:
  AsyncPullCallback(DefaultMQPushConsumerImpl* pushConsumer, boost::weak_ptr<PullRequest> request)
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

        uint64 pullRT = UtilAll::currentTimeMillis() - pullRequest->getLastPullTimestamp();
        StatsServerManager::getInstance()->getConsumeStatServer()->incConsumeRT(
            pullRequest->m_messageQueue.getTopic(), m_callbackOwner->getGroupName(), pullRT);
        pullRequest->setNextOffset(result.nextBeginOffset);
        pullRequest->putMessage(result.msgFoundList);
        if (!result.msgFoundList.empty()) {
          StatsServerManager::getInstance()->getConsumeStatServer()->incPullTPS(
              pullRequest->m_messageQueue.getTopic(), m_callbackOwner->getGroupName(), result.msgFoundList.size());
        }
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

        if ((pullRequest->getCacheMsgCount() == 0) && (result.nextBeginOffset > 0)) {
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

        if ((pullRequest->getCacheMsgCount() == 0) && (result.nextBeginOffset > 0)) {
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
  DefaultMQPushConsumerImpl* m_callbackOwner;
  boost::weak_ptr<PullRequest> m_pullRequest;
  bool m_bShutdown;
};

static boost::mutex m_asyncCallbackLock;

DefaultMQPushConsumerImpl::DefaultMQPushConsumerImpl() {}
DefaultMQPushConsumerImpl::DefaultMQPushConsumerImpl(const string& groupname)
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
  m_useNameSpaceMode = false;
  m_asyncPullTimeout = 30 * 1000;
  setMessageModel(CLUSTERING);

  m_startTime = UtilAll::currentTimeMillis();
  m_consumeThreadCount = std::thread::hardware_concurrency();
  m_pullMsgThreadPoolNum = std::thread::hardware_concurrency();
  m_async_service_thread.reset(new boost::thread(boost::bind(&DefaultMQPushConsumerImpl::boost_asio_work, this)));
}

void DefaultMQPushConsumerImpl::boost_asio_work() {
  LOG_INFO("DefaultMQPushConsumerImpl::boost asio async service runing");
  boost::asio::io_service::work work(m_async_ioService);  // avoid async io
  // service stops after
  // first timer timeout
  // callback
  m_async_ioService.run();
}

DefaultMQPushConsumerImpl::~DefaultMQPushConsumerImpl() {
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

bool DefaultMQPushConsumerImpl::sendMessageBack(MQMessageExt& msg, int delayLevel, string& brokerName) {
  string brokerAddr;
  if (!brokerName.empty())
    brokerAddr = getFactory()->findBrokerAddressInPublish(brokerName);
  else
    brokerAddr = socketAddress2IPPort(msg.getStoreHost());
  try {
    getFactory()->getMQClientAPIImpl()->consumerSendMessageBack(brokerAddr, msg, getGroupName(), delayLevel, 3000,
                                                                getMaxReconsumeTimes(), getSessionCredentials());
  } catch (MQException& e) {
    LOG_ERROR("%s", e.what());
    return false;
  }
  return true;
}

void DefaultMQPushConsumerImpl::fetchSubscribeMessageQueues(const string& topic, vector<MQMessageQueue>& mqs) {
  mqs.clear();
  try {
    getFactory()->fetchSubscribeMessageQueues(topic, mqs, getSessionCredentials());
  } catch (MQException& e) {
    LOG_ERROR("%s", e.what());
  }
}

void DefaultMQPushConsumerImpl::doRebalance() {
  if (isServiceStateOk()) {
    try {
      m_pRebalance->doRebalance();
    } catch (MQException& e) {
      LOG_ERROR("%s", e.what());
    }
  }
}

void DefaultMQPushConsumerImpl::persistConsumerOffset() {
  if (isServiceStateOk()) {
    m_pRebalance->persistConsumerOffset();
  }
}

void DefaultMQPushConsumerImpl::persistConsumerOffsetByResetOffset() {
  if (isServiceStateOk()) {
    m_pRebalance->persistConsumerOffsetByResetOffset();
  }
}

void DefaultMQPushConsumerImpl::start() {
#ifndef WIN32
  /* Ignore the SIGPIPE */
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, 0);
#endif
  LOG_WARN("###Current Push Consumer@%s", getClientVersionString().c_str());
  // deal with name space before start
  dealWithNameSpace();
  logConfigs();
  switch (m_serviceState) {
    case CREATE_JUST: {
      m_serviceState = START_FAILED;
      // Start status server
      StatsServerManager::getInstance()->getConsumeStatServer()->start();
      DefaultMQClient::start();
      dealWithMessageTrace();
      LOG_INFO("DefaultMQPushConsumerImpl:%s start", m_GroupName.c_str());

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
          new boost::thread(boost::bind(&DefaultMQPushConsumerImpl::runPullMsgQueue, this, m_pullmsgQueue)));

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

void DefaultMQPushConsumerImpl::shutdown() {
  switch (m_serviceState) {
    case RUNNING: {
      LOG_INFO("DefaultMQPushConsumerImpl shutdown");

      // Shutdown status server
      StatsServerManager::getInstance()->getConsumeStatServer()->shutdown();
      shutdownMessageTraceInnerProducer();
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

void DefaultMQPushConsumerImpl::registerMessageListener(MQMessageListener* pMessageListener) {
  if (NULL != pMessageListener) {
    m_pMessageListener = pMessageListener;
  }
}

MessageListenerType DefaultMQPushConsumerImpl::getMessageListenerType() {
  if (NULL != m_pMessageListener) {
    return m_pMessageListener->getMessageListenerType();
  }
  return messageListenerDefaultly;
}

ConsumeMsgService* DefaultMQPushConsumerImpl::getConsumerMsgService() const {
  return m_consumerService;
}

OffsetStore* DefaultMQPushConsumerImpl::getOffsetStore() const {
  return m_pOffsetStore;
}

Rebalance* DefaultMQPushConsumerImpl::getRebalance() const {
  return m_pRebalance;
}

void DefaultMQPushConsumerImpl::subscribe(const string& topic, const string& subExpression) {
  m_subTopics[topic] = subExpression;
}

void DefaultMQPushConsumerImpl::checkConfig() {
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

void DefaultMQPushConsumerImpl::copySubscription() {
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

void DefaultMQPushConsumerImpl::updateTopicSubscribeInfo(const string& topic, vector<MQMessageQueue>& info) {
  m_pRebalance->setTopicSubscribeInfo(topic, info);
}

void DefaultMQPushConsumerImpl::updateTopicSubscribeInfoWhenSubscriptionChanged() {
  map<string, SubscriptionData*>& subTable = m_pRebalance->getSubscriptionInner();
  map<string, SubscriptionData*>::iterator it = subTable.begin();
  for (; it != subTable.end(); ++it) {
    bool btopic = getFactory()->updateTopicRouteInfoFromNameServer(it->first, getSessionCredentials());
    if (btopic == false) {
      LOG_WARN("The topic:[%s] not exist", it->first.c_str());
    }
  }
}

ConsumeType DefaultMQPushConsumerImpl::getConsumeType() {
  return CONSUME_PASSIVELY;
}

ConsumeFromWhere DefaultMQPushConsumerImpl::getConsumeFromWhere() {
  return m_consumeFromWhere;
}

void DefaultMQPushConsumerImpl::setConsumeFromWhere(ConsumeFromWhere consumeFromWhere) {
  m_consumeFromWhere = consumeFromWhere;
}

void DefaultMQPushConsumerImpl::getSubscriptions(vector<SubscriptionData>& result) {
  map<string, SubscriptionData*>& subTable = m_pRebalance->getSubscriptionInner();
  map<string, SubscriptionData*>::iterator it = subTable.begin();
  for (; it != subTable.end(); ++it) {
    result.push_back(*(it->second));
  }
}

void DefaultMQPushConsumerImpl::updateConsumeOffset(const MQMessageQueue& mq, int64 offset) {
  if (offset >= 0) {
    m_pOffsetStore->updateOffset(mq, offset);
  } else {
    LOG_ERROR("updateConsumeOffset of mq:%s error", mq.toString().c_str());
  }
}

void DefaultMQPushConsumerImpl::removeConsumeOffset(const MQMessageQueue& mq) {
  m_pOffsetStore->removeOffset(mq);
}

void DefaultMQPushConsumerImpl::static_triggerNextPullRequest(void* context,
                                                              boost::asio::deadline_timer* t,
                                                              boost::weak_ptr<PullRequest> pullRequest) {
  if (pullRequest.expired()) {
    LOG_WARN("Pull request has been released before.");
    return;
  }
  DefaultMQPushConsumerImpl* pDefaultMQPushConsumerImpl = (DefaultMQPushConsumerImpl*)context;
  if (pDefaultMQPushConsumerImpl) {
    pDefaultMQPushConsumerImpl->triggerNextPullRequest(t, pullRequest);
  }
}

void DefaultMQPushConsumerImpl::triggerNextPullRequest(boost::asio::deadline_timer* t,
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

bool DefaultMQPushConsumerImpl::producePullMsgTaskLater(boost::weak_ptr<PullRequest> pullRequest, int millis) {
  boost::shared_ptr<PullRequest> request = pullRequest.lock();
  if (!request) {
    LOG_INFO("Pull request is invalid. Maybe it is dropped before.");
    return false;
  }
  if (request->isDropped()) {
    LOG_INFO("[Dropped]Remove pullmsg event of mq:%s", request->m_messageQueue.toString().c_str());
    return false;
  }
  if (m_pullmsgQueue->bTaskQueueStatusOK() && isServiceStateOk()) {
    boost::asio::deadline_timer* t =
        new boost::asio::deadline_timer(m_async_ioService, boost::posix_time::milliseconds(millis));
    t->async_wait(boost::bind(&(DefaultMQPushConsumerImpl::static_triggerNextPullRequest), this, t, request));
    LOG_INFO("Produce Pull request [%s] Later and Sleep [%d]ms.", (request->m_messageQueue).toString().c_str(), millis);
    return true;
  } else {
    LOG_WARN("Service or TaskQueue shutdown, produce PullRequest of mq:%s failed",
             request->m_messageQueue.toString().c_str());
    return false;
  }
}

bool DefaultMQPushConsumerImpl::producePullMsgTask(boost::weak_ptr<PullRequest> pullRequest) {
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
      m_pullmsgQueue->produce(TaskBinder::gen(&DefaultMQPushConsumerImpl::pullMessageAsync, this, request));
    } else {
      m_pullmsgQueue->produce(TaskBinder::gen(&DefaultMQPushConsumerImpl::pullMessage, this, request));
    }
  } else {
    LOG_WARN("produce PullRequest of mq:%s failed", request->m_messageQueue.toString().c_str());
    return false;
  }
  return true;
}

void DefaultMQPushConsumerImpl::runPullMsgQueue(TaskQueue* pTaskQueue) {
  pTaskQueue->run();
}

void DefaultMQPushConsumerImpl::pullMessage(boost::weak_ptr<PullRequest> pullRequest) {
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
    uint64 startTimeStamp = UtilAll::currentTimeMillis();
    request->setLastPullTimestamp(startTimeStamp);
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
        uint64 pullRT = UtilAll::currentTimeMillis() - startTimeStamp;
        StatsServerManager::getInstance()->getConsumeStatServer()->incConsumeRT(messageQueue.getTopic(), getGroupName(),
                                                                                pullRT);
        if (request->isDropped()) {
          LOG_INFO("Get pull result but the queue has been marked as dropped. Queue: %s",
                   messageQueue.toString().c_str());
          break;
        }
        // and this request is dropped, and then received pulled msgs.
        request->setNextOffset(pullResult.nextBeginOffset);
        request->putMessage(pullResult.msgFoundList);
        if (!pullResult.msgFoundList.empty()) {
          StatsServerManager::getInstance()->getConsumeStatServer()->incPullTPS(messageQueue.getTopic(), getGroupName(),
                                                                                pullResult.msgFoundList.size());
        }
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
        if ((request->getCacheMsgCount() == 0) && (pullResult.nextBeginOffset > 0)) {
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
        if ((request->getCacheMsgCount() == 0) && (pullResult.nextBeginOffset > 0)) {
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
    LOG_ERROR("%s", e.what());
    LOG_WARN("Pull %s occur exception, restart 1s  later.", messageQueue.toString().c_str());
    producePullMsgTaskLater(request, 1000);
  }
}

AsyncPullCallback* DefaultMQPushConsumerImpl::getAsyncPullCallBack(boost::weak_ptr<PullRequest> pullRequest,
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
    if (asyncPullCallback) {
      // maybe the pull request has dropped before, replace event time.
      asyncPullCallback->setPullRequest(pullRequest);
    }
    return asyncPullCallback;
  }

  return NULL;
}

void DefaultMQPushConsumerImpl::shutdownAsyncPullCallBack() {
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

void DefaultMQPushConsumerImpl::pullMessageAsync(boost::weak_ptr<PullRequest> pullRequest) {
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
    LOG_ERROR("%s", e.what());
    if (request->isDropped()) {
      LOG_WARN("Pull request is set as dropped with mq:%s, return", (request->m_messageQueue).toString().c_str());
      return;
    }
    LOG_INFO("Pull %s occur exception, restart 1s  later.", (request->m_messageQueue).toString().c_str());
    producePullMsgTaskLater(request, 1000);
  }
}

void DefaultMQPushConsumerImpl::setAsyncPull(bool asyncFlag) {
  if (asyncFlag) {
    LOG_INFO("set pushConsumer:%s to async default pull mode", getGroupName().c_str());
  } else {
    LOG_INFO("set pushConsumer:%s to sync pull mode", getGroupName().c_str());
  }
  m_asyncPull = asyncFlag;
}

void DefaultMQPushConsumerImpl::setConsumeThreadCount(int threadCount) {
  if (threadCount > 0) {
    m_consumeThreadCount = threadCount;
  } else {
    LOG_ERROR("setConsumeThreadCount with invalid value");
  }
}

int DefaultMQPushConsumerImpl::getConsumeThreadCount() const {
  return m_consumeThreadCount;
}
void DefaultMQPushConsumerImpl::setMaxReconsumeTimes(int maxReconsumeTimes) {
  if (maxReconsumeTimes > 0) {
    m_maxReconsumeTimes = maxReconsumeTimes;
  } else {
    LOG_ERROR("set maxReconsumeTimes with invalid value");
  }
}

int DefaultMQPushConsumerImpl::getMaxReconsumeTimes() const {
  if (m_maxReconsumeTimes >= 0) {
    return m_maxReconsumeTimes;
  }
  // return 16 as default;
  return 16;
}

void DefaultMQPushConsumerImpl::setPullMsgThreadPoolCount(int threadCount) {
  m_pullMsgThreadPoolNum = threadCount;
}

int DefaultMQPushConsumerImpl::getPullMsgThreadPoolCount() const {
  return m_pullMsgThreadPoolNum;
}

int DefaultMQPushConsumerImpl::getConsumeMessageBatchMaxSize() const {
  return m_consumeMessageBatchMaxSize;
}

void DefaultMQPushConsumerImpl::setConsumeMessageBatchMaxSize(int consumeMessageBatchMaxSize) {
  if (consumeMessageBatchMaxSize >= 1)
    m_consumeMessageBatchMaxSize = consumeMessageBatchMaxSize;
}

void DefaultMQPushConsumerImpl::setMaxCacheMsgSizePerQueue(int maxCacheSize) {
  if (maxCacheSize > 0 && maxCacheSize < 65535) {
    LOG_INFO("set maxCacheSize to:%d for consumer:%s", maxCacheSize, getGroupName().c_str());
    m_maxMsgCacheSize = maxCacheSize;
  }
}

int DefaultMQPushConsumerImpl::getMaxCacheMsgSizePerQueue() const {
  return m_maxMsgCacheSize;
}

ConsumerRunningInfo* DefaultMQPushConsumerImpl::getConsumerRunningInfo() {
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

  for (const auto& it : result) {
    ConsumeStats consumeStat;
    // we should get it from status service.
    consumeStat =
        StatsServerManager::getInstance()->getConsumeStatServer()->getConsumeStats(it.getTopic(), this->getGroupName());
    info->setStatusTable(it.getTopic(), consumeStat);
  }

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
// we should deal with name space before producer start.
bool DefaultMQPushConsumerImpl::dealWithNameSpace() {
  string ns = getNameSpace();
  if (ns.empty()) {
    string nsAddr = getNamesrvAddr();
    if (!NameSpaceUtil::checkNameSpaceExistInNameServer(nsAddr)) {
      return true;
    }
    ns = NameSpaceUtil::getNameSpaceFromNsURL(nsAddr);
    // reset namespace
    setNameSpace(ns);
  }
  // reset group name
  if (!NameSpaceUtil::hasNameSpace(getGroupName(), ns)) {
    string fullGID = NameSpaceUtil::withNameSpace(getGroupName(), ns);
    setGroupName(fullGID);
  }
  map<string, string> subTmp;
  map<string, string>::iterator it = m_subTopics.begin();
  for (; it != m_subTopics.end(); ++it) {
    string topic = it->first;
    string subs = it->second;
    if (!NameSpaceUtil::hasNameSpace(topic, ns)) {
      LOG_INFO("Update Subscribe[%s:%s] with NameSpace:%s", it->first.c_str(), it->second.c_str(), ns.c_str());
      topic = NameSpaceUtil::withNameSpace(topic, ns);
      // let other mode to known, the name space model opened.
      m_useNameSpaceMode = true;
    }
    subTmp[topic] = subs;
  }
  m_subTopics.swap(subTmp);

  return true;
}
void DefaultMQPushConsumerImpl::logConfigs() {
  showClientConfigs();

  LOG_WARN("MessageModel:%d", m_messageModel);
  LOG_WARN("MessageModel:%s", m_messageModel == BROADCASTING ? "BROADCASTING" : "CLUSTERING");

  LOG_WARN("ConsumeFromWhere:%d", m_consumeFromWhere);
  switch (m_consumeFromWhere) {
    case CONSUME_FROM_FIRST_OFFSET:
      LOG_WARN("ConsumeFromWhere:%s", "CONSUME_FROM_FIRST_OFFSET");
      break;
    case CONSUME_FROM_LAST_OFFSET:
      LOG_WARN("ConsumeFromWhere:%s", "CONSUME_FROM_LAST_OFFSET");
      break;

    case CONSUME_FROM_TIMESTAMP:
      LOG_WARN("ConsumeFromWhere:%s", "CONSUME_FROM_TIMESTAMP");
      break;
    case CONSUME_FROM_LAST_OFFSET_AND_FROM_MIN_WHEN_BOOT_FIRST:
      LOG_WARN("ConsumeFromWhere:%s", "CONSUME_FROM_LAST_OFFSET_AND_FROM_MIN_WHEN_BOOT_FIRST");
      break;
    case CONSUME_FROM_MAX_OFFSET:
      LOG_WARN("ConsumeFromWhere:%s", "CONSUME_FROM_MAX_OFFSET");
      break;
    case CONSUME_FROM_MIN_OFFSET:
      LOG_WARN("ConsumeFromWhere:%s", "CONSUME_FROM_MAX_OFFSET");
      break;
    default:
      LOG_WARN("ConsumeFromWhere:%s", "UnKnown.");
      break;
  }
  LOG_WARN("ConsumeThreadCount:%d", m_consumeThreadCount);
  LOG_WARN("ConsumeMessageBatchMaxSize:%d", m_consumeMessageBatchMaxSize);
  LOG_WARN("MaxMsgCacheSizePerQueue:%d", m_maxMsgCacheSize);
  LOG_WARN("MaxReconsumeTimes:%d", m_maxReconsumeTimes);
  LOG_WARN("PullMsgThreadPoolNum:%d", m_pullMsgThreadPoolNum);
  LOG_WARN("AsyncPullMode:%s", m_asyncPull ? "true" : "false");
  LOG_WARN("AsyncPullTimeout:%d ms", m_asyncPullTimeout);
}
// we should create trace message poll before producer send messages.
bool DefaultMQPushConsumerImpl::dealWithMessageTrace() {
  if (!getMessageTrace()) {
    LOG_INFO("Message Trace set to false, Will not send trace messages.");
    return false;
  }
  // Try to create default producer inner.
  LOG_INFO("DefaultMQPushConsumer Open message trace..");

  createMessageTraceInnerProducer();
  std::shared_ptr<ConsumeMessageHook> hook(new ConsumeMessageHookImpl());
  registerConsumeMessageHook(hook);
  return true;
}

void DefaultMQPushConsumerImpl::createMessageTraceInnerProducer() {
  m_DefaultMQProducerImpl = std::make_shared<DefaultMQProducerImpl>(getGroupName());
  m_DefaultMQProducerImpl->setMessageTrace(false);
  m_DefaultMQProducerImpl->setInstanceName("MESSAGE_TRACE_" + getInstanceName());
  const SessionCredentials& session = getSessionCredentials();
  m_DefaultMQProducerImpl->setSessionCredentials(session.getAccessKey(), session.getSecretKey(),
                                                 session.getAuthChannel());
  if (!getNamesrvAddr().empty()) {
    m_DefaultMQProducerImpl->setNamesrvAddr(getNamesrvAddr());
  }
  m_DefaultMQProducerImpl->setNameSpace(getNameSpace());
  // m_DefaultMQProducerImpl->setNamesrvDomain(getNamesrvDomain());
  m_DefaultMQProducerImpl->start();
}
void DefaultMQPushConsumerImpl::shutdownMessageTraceInnerProducer() {
  if (!getMessageTrace()) {
    return;
  }
  if (m_DefaultMQProducerImpl) {
    LOG_INFO("Shutdown Message Trace Inner Producer In Consumer.");
    m_DefaultMQProducerImpl->shutdown();
  }
}
bool DefaultMQPushConsumerImpl::hasConsumeMessageHook() {
  return !m_consumeMessageHookList.empty();
}

void DefaultMQPushConsumerImpl::registerConsumeMessageHook(std::shared_ptr<ConsumeMessageHook>& hook) {
  m_consumeMessageHookList.push_back(hook);
  LOG_INFO("Register ConsumeMessageHook success,hookname is %s", hook->getHookName().c_str());
}

void DefaultMQPushConsumerImpl::executeConsumeMessageHookBefore(ConsumeMessageContext* context) {
  if (!m_consumeMessageHookList.empty()) {
    std::vector<std::shared_ptr<ConsumeMessageHook>>::iterator it = m_consumeMessageHookList.begin();
    for (; it != m_consumeMessageHookList.end(); ++it) {
      try {
        (*it)->executeHookBefore(context);
      } catch (exception e) {
      }
    }
  }
}

void DefaultMQPushConsumerImpl::executeConsumeMessageHookAfter(ConsumeMessageContext* context) {
  if (!m_consumeMessageHookList.empty()) {
    std::vector<std::shared_ptr<ConsumeMessageHook>>::iterator it = m_consumeMessageHookList.begin();
    for (; it != m_consumeMessageHookList.end(); ++it) {
      try {
        (*it)->executeHookAfter(context);
      } catch (exception e) {
      }
    }
  }
}

void DefaultMQPushConsumerImpl::submitSendTraceRequest(MQMessage& msg, SendCallback* pSendCallback) {
  if (getMessageTrace()) {
    try {
      LOG_DEBUG("=====Send Trace Messages,Topic[%s],Key[%s],Body[%s]", msg.getTopic().c_str(), msg.getKeys().c_str(),
                msg.getBody().c_str());
      // m_DefaultMQProducerImpl->submitSendTraceRequest(msg, pSendCallback);
      m_DefaultMQProducerImpl->send(msg, pSendCallback, false);
    } catch (exception e) {
      LOG_INFO(e.what());
    }
  }
}

void DefaultMQPushConsumerImpl::setDefaultMqProducerImpl(DefaultMQProducerImpl* DefaultMqProducerImpl) {
  m_DefaultMQProducerImpl.reset(DefaultMqProducerImpl);
}
}  // namespace rocketmq
