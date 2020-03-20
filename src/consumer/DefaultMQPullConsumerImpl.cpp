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

#include "DefaultMQPullConsumerImpl.h"
#include "AsyncArg.h"
#include "CommunicationMode.h"
#include "FilterAPI.h"
#include "Logging.h"
#include "MQClientFactory.h"
#include "MessageAccessor.h"
#include "NameSpaceUtil.h"
#include "OffsetStore.h"
#include "PullAPIWrapper.h"
#include "PullSysFlag.h"
#include "Rebalance.h"
#include "Validators.h"

namespace rocketmq {
//<!***************************************************************************
DefaultMQPullConsumerImpl::DefaultMQPullConsumerImpl(const string& groupname)
    : m_pMessageQueueListener(NULL),
      m_pOffsetStore(NULL),
      m_pRebalance(NULL),
      m_pPullAPIWrapper(NULL)

{
  //<!set default group name;
  string gname = groupname.empty() ? DEFAULT_CONSUMER_GROUP : groupname;
  setGroupName(gname);

  setMessageModel(CLUSTERING);
}

DefaultMQPullConsumerImpl::~DefaultMQPullConsumerImpl() {
  m_pMessageQueueListener = NULL;
  deleteAndZero(m_pRebalance);
  deleteAndZero(m_pOffsetStore);
  deleteAndZero(m_pPullAPIWrapper);
}

// MQConsumer
//<!************************************************************************
void DefaultMQPullConsumerImpl::start() {
#ifndef WIN32
  /* Ignore the SIGPIPE */
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  sigaction(SIGPIPE, &sa, 0);
#endif
  LOG_INFO("###Current Pull Consumer@%s", getClientVersionString().c_str());
  dealWithNameSpace();
  showClientConfigs();
  switch (m_serviceState) {
    case CREATE_JUST: {
      m_serviceState = START_FAILED;
      DefaultMQClient::start();
      LOG_INFO("DefaultMQPullConsumerImpl:%s start", m_GroupName.c_str());

      //<!create rebalance;
      m_pRebalance = new RebalancePull(this, getFactory());

      string groupname = getGroupName();
      m_pPullAPIWrapper = new PullAPIWrapper(getFactory(), groupname);

      //<!data;
      checkConfig();
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

      getFactory()->start();
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
}

void DefaultMQPullConsumerImpl::shutdown() {
  switch (m_serviceState) {
    case RUNNING: {
      LOG_INFO("DefaultMQPullConsumerImpl:%s shutdown", m_GroupName.c_str());
      persistConsumerOffset();
      getFactory()->unregisterConsumer(this);
      getFactory()->shutdown();
      m_serviceState = SHUTDOWN_ALREADY;
      break;
    }
    case SHUTDOWN_ALREADY:
    case CREATE_JUST:
      break;
    default:
      break;
  }
}

bool DefaultMQPullConsumerImpl::sendMessageBack(MQMessageExt& msg, int delayLevel, string& brokerName) {
  return true;
}

void DefaultMQPullConsumerImpl::fetchSubscribeMessageQueues(const string& topic, vector<MQMessageQueue>& mqs) {
  mqs.clear();
  try {
    const string localTopic = NameSpaceUtil::withNameSpace(topic, getNameSpace());
    getFactory()->fetchSubscribeMessageQueues(localTopic, mqs, getSessionCredentials());
  } catch (MQException& e) {
    LOG_ERROR("%s", e.what());
  }
}

void DefaultMQPullConsumerImpl::updateTopicSubscribeInfo(const string& topic, vector<MQMessageQueue>& info) {}

void DefaultMQPullConsumerImpl::registerMessageQueueListener(const string& topic, MQueueListener* pListener) {
  m_registerTopics.insert(topic);
  if (pListener) {
    m_pMessageQueueListener = pListener;
  }
}

PullResult DefaultMQPullConsumerImpl::pull(const MQMessageQueue& mq,
                                           const string& subExpression,
                                           int64 offset,
                                           int maxNums) {
  return pullSyncImpl(mq, subExpression, offset, maxNums, false);
}

void DefaultMQPullConsumerImpl::pull(const MQMessageQueue& mq,
                                     const string& subExpression,
                                     int64 offset,
                                     int maxNums,
                                     PullCallback* pPullCallback) {
  pullAsyncImpl(mq, subExpression, offset, maxNums, false, pPullCallback);
}

PullResult DefaultMQPullConsumerImpl::pullBlockIfNotFound(const MQMessageQueue& mq,
                                                          const string& subExpression,
                                                          int64 offset,
                                                          int maxNums) {
  return pullSyncImpl(mq, subExpression, offset, maxNums, true);
}

void DefaultMQPullConsumerImpl::pullBlockIfNotFound(const MQMessageQueue& mq,
                                                    const string& subExpression,
                                                    int64 offset,
                                                    int maxNums,
                                                    PullCallback* pPullCallback) {
  pullAsyncImpl(mq, subExpression, offset, maxNums, true, pPullCallback);
}

PullResult DefaultMQPullConsumerImpl::pullSyncImpl(const MQMessageQueue& mq,
                                                   const string& subExpression,
                                                   int64 offset,
                                                   int maxNums,
                                                   bool block) {
  if (offset < 0)
    THROW_MQEXCEPTION(MQClientException, "offset < 0", -1);

  if (maxNums <= 0)
    THROW_MQEXCEPTION(MQClientException, "maxNums <= 0", -1);

  //<!auto subscript,all sub;
  subscriptionAutomatically(mq.getTopic());

  int sysFlag = PullSysFlag::buildSysFlag(false, block, true, false);

  //<!this sub;
  unique_ptr<SubscriptionData> pSData(FilterAPI::buildSubscriptionData(mq.getTopic(), subExpression));

  int timeoutMillis = block ? 1000 * 30 : 1000 * 10;

  try {
    unique_ptr<PullResult> pullResult(m_pPullAPIWrapper->pullKernelImpl(mq,                      // 1
                                                                        pSData->getSubString(),  // 2
                                                                        0L,                      // 3
                                                                        offset,                  // 4
                                                                        maxNums,                 // 5
                                                                        sysFlag,                 // 6
                                                                        0,                       // 7
                                                                        1000 * 20,               // 8
                                                                        timeoutMillis,           // 9
                                                                        ComMode_SYNC,            // 10
                                                                        NULL,                    //<!callback;
                                                                        getSessionCredentials(), NULL));
    PullResult pr = m_pPullAPIWrapper->processPullResult(mq, pullResult.get(), pSData.get());
    if (m_useNameSpaceMode) {
      MessageAccessor::withoutNameSpace(pr.msgFoundList, m_nameSpace);
    }
    return pr;
  } catch (MQException& e) {
    LOG_ERROR("%s", e.what());
  }
  return PullResult(BROKER_TIMEOUT);
}

void DefaultMQPullConsumerImpl::pullAsyncImpl(const MQMessageQueue& mq,
                                              const string& subExpression,
                                              int64 offset,
                                              int maxNums,
                                              bool block,
                                              PullCallback* pPullCallback) {
  if (offset < 0)
    THROW_MQEXCEPTION(MQClientException, "offset < 0", -1);

  if (maxNums <= 0)
    THROW_MQEXCEPTION(MQClientException, "maxNums <= 0", -1);

  if (!pPullCallback)
    THROW_MQEXCEPTION(MQClientException, "pPullCallback is null", -1);

  //<!auto subscript,all sub;
  subscriptionAutomatically(mq.getTopic());

  int sysFlag = PullSysFlag::buildSysFlag(false, block, true, false);

  //<!this sub;
  unique_ptr<SubscriptionData> pSData(FilterAPI::buildSubscriptionData(mq.getTopic(), subExpression));

  int timeoutMillis = block ? 1000 * 30 : 1000 * 10;

  //<!�첽����;
  AsyncArg arg;
  arg.mq = mq;
  arg.subData = *pSData;
  arg.pPullWrapper = m_pPullAPIWrapper;

  try {
    // not support name space
    unique_ptr<PullResult> pullResult(m_pPullAPIWrapper->pullKernelImpl(mq,                      // 1
                                                                        pSData->getSubString(),  // 2
                                                                        0L,                      // 3
                                                                        offset,                  // 4
                                                                        maxNums,                 // 5
                                                                        sysFlag,                 // 6
                                                                        0,                       // 7
                                                                        1000 * 20,               // 8
                                                                        timeoutMillis,           // 9
                                                                        ComMode_ASYNC,           // 10
                                                                        pPullCallback, getSessionCredentials(), &arg));
  } catch (MQException& e) {
    LOG_ERROR("%s", e.what());
  }
}

void DefaultMQPullConsumerImpl::subscriptionAutomatically(const string& topic) {
  SubscriptionData* pSdata = m_pRebalance->getSubscriptionData(topic);
  if (pSdata == NULL) {
    unique_ptr<SubscriptionData> subscriptionData(FilterAPI::buildSubscriptionData(topic, SUB_ALL));
    m_pRebalance->setSubscriptionData(topic, subscriptionData.release());
  }
}

void DefaultMQPullConsumerImpl::updateConsumeOffset(const MQMessageQueue& mq, int64 offset) {
  m_pOffsetStore->updateOffset(mq, offset);
}

void DefaultMQPullConsumerImpl::removeConsumeOffset(const MQMessageQueue& mq) {
  m_pOffsetStore->removeOffset(mq);
}

int64 DefaultMQPullConsumerImpl::fetchConsumeOffset(const MQMessageQueue& mq, bool fromStore) {
  return m_pOffsetStore->readOffset(mq, fromStore ? READ_FROM_STORE : MEMORY_FIRST_THEN_STORE, getSessionCredentials());
}

void DefaultMQPullConsumerImpl::persistConsumerOffset() {
  /*As do not execute rebalance for pullConsumer now, requestTable is always
  empty
  map<MQMessageQueue, PullRequest*> requestTable =
  m_pRebalance->getPullRequestTable();
  map<MQMessageQueue, PullRequest*>::iterator it = requestTable.begin();
  vector<MQMessageQueue> mqs;
  for (; it != requestTable.end(); ++it)
  {
      if (it->second)
      {
          mqs.push_back(it->first);
      }
  }
  m_pOffsetStore->persistAll(mqs);*/
}

void DefaultMQPullConsumerImpl::persistConsumerOffsetByResetOffset() {}

void DefaultMQPullConsumerImpl::persistConsumerOffset4PullConsumer(const MQMessageQueue& mq) {
  if (isServiceStateOk()) {
    m_pOffsetStore->persist(mq, getSessionCredentials());
  }
}

void DefaultMQPullConsumerImpl::fetchMessageQueuesInBalance(const string& topic, vector<MQMessageQueue> mqs) {}

void DefaultMQPullConsumerImpl::checkConfig() {
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
}

void DefaultMQPullConsumerImpl::doRebalance() {}

void DefaultMQPullConsumerImpl::copySubscription() {
  set<string>::iterator it = m_registerTopics.begin();
  for (; it != m_registerTopics.end(); ++it) {
    unique_ptr<SubscriptionData> subscriptionData(FilterAPI::buildSubscriptionData((*it), SUB_ALL));
    m_pRebalance->setSubscriptionData((*it), subscriptionData.release());
  }
}

ConsumeType DefaultMQPullConsumerImpl::getConsumeType() {
  return CONSUME_ACTIVELY;
}

ConsumeFromWhere DefaultMQPullConsumerImpl::getConsumeFromWhere() {
  return CONSUME_FROM_LAST_OFFSET;
}

void DefaultMQPullConsumerImpl::getSubscriptions(vector<SubscriptionData>& result) {
  set<string>::iterator it = m_registerTopics.begin();
  for (; it != m_registerTopics.end(); ++it) {
    SubscriptionData ms(*it, SUB_ALL);
    result.push_back(ms);
  }
}

bool DefaultMQPullConsumerImpl::producePullMsgTask(boost::weak_ptr<PullRequest> pullRequest) {
  return true;
}

Rebalance* DefaultMQPullConsumerImpl::getRebalance() const {
  return NULL;
}
// we should deal with name space before producer start.
bool DefaultMQPullConsumerImpl::dealWithNameSpace() {
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
  set<string> tmpTopics;
  for (auto iter = m_registerTopics.begin(); iter != m_registerTopics.end(); iter++) {
    string topic = *iter;
    if (!NameSpaceUtil::hasNameSpace(topic, ns)) {
      LOG_INFO("Update Subscribe Topic[%s] with NameSpace:%s", topic.c_str(), ns.c_str());
      topic = NameSpaceUtil::withNameSpace(topic, ns);
      // let other mode to known, the name space model opened.
      m_useNameSpaceMode = true;
    }
    tmpTopics.insert(topic);
  }
  m_registerTopics.swap(tmpTopics);
  return true;
}
//<!************************************************************************
}  // namespace rocketmq
