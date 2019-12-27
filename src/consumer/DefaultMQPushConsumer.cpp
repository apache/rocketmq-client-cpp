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

#ifndef WIN32
#include <signal.h>
#endif

#include "AllocateMQAveragely.h"
#include "CommunicationMode.h"
#include "ConsumeMsgService.h"
#include "ConsumerRunningInfo.h"
#include "FilterAPI.h"
#include "Logging.h"
#include "MQAdminImpl.h"
#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "MQClientManager.h"
#include "MQProtos.h"
#include "OffsetStore.h"
#include "PullAPIWrapper.h"
#include "PullMessageService.h"
#include "PullSysFlag.h"
#include "RebalanceImpl.h"
#include "UtilAll.h"
#include "Validators.h"

namespace rocketmq {

class AsyncPullCallback : public AutoDeletePullCallback {
 public:
  AsyncPullCallback(DefaultMQPushConsumerPtr pushConsumer, PullRequestPtr request, SubscriptionDataPtr subscriptionData)
      : m_defaultMQPushConsumer(pushConsumer), m_pullRequest(request), m_subscriptionData(subscriptionData) {}

  ~AsyncPullCallback() override {
    m_defaultMQPushConsumer.reset();
    m_pullRequest.reset();
    m_subscriptionData = nullptr;
  }

  void onSuccess(PullResult& pullResult) override {
    auto defaultMQPushConsumer = m_defaultMQPushConsumer.lock();
    if (nullptr == defaultMQPushConsumer) {
      LOG_WARN_NEW("AsyncPullCallback::onSuccess: DefaultMQPushConsumer is released.");
      return;
    }

    PullResult result = defaultMQPushConsumer->getPullAPIWrapper()->processPullResult(m_pullRequest->getMessageQueue(),
                                                                                      pullResult, m_subscriptionData);
    switch (result.pullStatus) {
      case FOUND: {
        int64_t prevRequestOffset = m_pullRequest->getNextOffset();
        m_pullRequest->setNextOffset(result.nextBeginOffset);

        int64_t firstMsgOffset = (std::numeric_limits<int64_t>::max)();
        if (result.msgFoundList.empty()) {
          defaultMQPushConsumer->executePullRequestImmediately(m_pullRequest);
        } else {
          firstMsgOffset = (*result.msgFoundList.begin())->getQueueOffset();

          m_pullRequest->getProcessQueue()->putMessage(result.msgFoundList);
          defaultMQPushConsumer->getConsumerMsgService()->submitConsumeRequest(
              result.msgFoundList, m_pullRequest->getProcessQueue(), m_pullRequest->getMessageQueue(), true);

          defaultMQPushConsumer->executePullRequestImmediately(m_pullRequest);
        }

        if (result.nextBeginOffset < prevRequestOffset || firstMsgOffset < prevRequestOffset) {
          LOG_WARN_NEW(
              "[BUG] pull message result maybe data wrong, nextBeginOffset:{} firstMsgOffset:{} prevRequestOffset:{}",
              result.nextBeginOffset, firstMsgOffset, prevRequestOffset);
        }

      } break;
      case NO_NEW_MSG:
      case NO_MATCHED_MSG:
        m_pullRequest->setNextOffset(result.nextBeginOffset);
        defaultMQPushConsumer->correctTagsOffset(m_pullRequest);
        defaultMQPushConsumer->executePullRequestImmediately(m_pullRequest);
        break;
      case OFFSET_ILLEGAL: {
        LOG_WARN_NEW("the pull request offset illegal, {} {}", m_pullRequest->toString(), result.toString());

        m_pullRequest->setNextOffset(result.nextBeginOffset);
        m_pullRequest->getProcessQueue()->setDropped(true);

        // update and persist offset, then removeProcessQueue
        auto pullRequest = m_pullRequest;
        defaultMQPushConsumer->executeTaskLater(
            [defaultMQPushConsumer, pullRequest]() {
              try {
                defaultMQPushConsumer->getOffsetStore()->updateOffset(pullRequest->getMessageQueue(),
                                                                      pullRequest->getNextOffset(), false);
                defaultMQPushConsumer->getOffsetStore()->persist(pullRequest->getMessageQueue());
                defaultMQPushConsumer->getRebalanceImpl()->removeProcessQueue(pullRequest->getMessageQueue());

                LOG_WARN_NEW("fix the pull request offset, {}", pullRequest->toString());
              } catch (std::exception& e) {
                LOG_ERROR_NEW("executeTaskLater Exception: {}", e.what());
              }
            },
            10000);
      } break;
      default:
        break;
    }
  }

  void onException(MQException& e) noexcept override {
    auto defaultMQPushConsumer = m_defaultMQPushConsumer.lock();
    if (nullptr == defaultMQPushConsumer) {
      LOG_WARN_NEW("AsyncPullCallback::onException: DefaultMQPushConsumer is released.");
      return;
    }

    if (!UtilAll::isRetryTopic(m_pullRequest->getMessageQueue().getTopic())) {
      LOG_WARN_NEW("execute the pull request exception: {}", e.what());
    }

    defaultMQPushConsumer->executePullRequestLater(m_pullRequest, 3000);
  }

 private:
  std::weak_ptr<DefaultMQPushConsumer> m_defaultMQPushConsumer;
  PullRequestPtr m_pullRequest;
  SubscriptionDataPtr m_subscriptionData;
};

DefaultMQPushConsumerConfig::DefaultMQPushConsumerConfig()
    : m_consumeFromWhere(CONSUME_FROM_LAST_OFFSET),
      m_consumeThreadNum(std::min(8, (int)std::thread::hardware_concurrency())),
      m_consumeMessageBatchMaxSize(1),
      m_maxMsgCacheSize(1000),
      m_asyncPullTimeout(30 * 1000),
      m_allocateMQStrategy(new AllocateMQAveragely()) {}

DefaultMQPushConsumer::DefaultMQPushConsumer(const string& groupname) : DefaultMQPushConsumer(groupname, nullptr) {}

DefaultMQPushConsumer::DefaultMQPushConsumer(const string& groupname, std::shared_ptr<RPCHook> rpcHook)
    : MQClient(rpcHook),
      m_startTime(UtilAll::currentTimeMillis()),
      m_pause(false),
      m_consumeOrderly(false),
      m_rebalanceImpl(new RebalancePushImpl(this)),
      m_pullAPIWrapper(nullptr),
      m_offsetStore(nullptr),
      m_consumerService(nullptr),
      m_messageListener(nullptr) {
  // set default group name
  if (groupname.empty()) {
    setGroupName(DEFAULT_CONSUMER_GROUP);
  } else {
    setGroupName(groupname);
  }
}

DefaultMQPushConsumer::~DefaultMQPushConsumer() = default;

bool DefaultMQPushConsumer::sendMessageBack(MQMessageExt& msg, int delayLevel) {
  try {
    m_clientInstance->getMQClientAPIImpl()->consumerSendMessageBack(msg, getGroupName(), delayLevel, 3000);
    return true;
  } catch (std::exception& e) {
    LOG_ERROR_NEW("sendMessageBack exception, group: {}, msg: {}. {}", m_groupName, msg.toString(), e.what());
  }
  return false;
}

void DefaultMQPushConsumer::fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) {
  mqs.clear();
  try {
    m_clientInstance->getMQAdminImpl()->fetchSubscribeMessageQueues(topic, mqs);
  } catch (MQException& e) {
    LOG_ERROR_NEW("{}", e.what());
  }
}

void DefaultMQPushConsumer::doRebalance() {
  if (!m_pause) {
    m_rebalanceImpl->doRebalance(isConsumeOrderly());
  }
}

void DefaultMQPushConsumer::persistConsumerOffset() {
  if (isServiceStateOk()) {
    std::vector<MQMessageQueue> mqs = m_rebalanceImpl->getAllocatedMQ();
    if (getMessageModel() == BROADCASTING) {
      m_offsetStore->persistAll(mqs);
    } else {
      for (const auto& mq : mqs) {
        m_offsetStore->persist(mq);
      }
    }
  }
}

void DefaultMQPushConsumer::start() {
#ifndef WIN32
  /* Ignore the SIGPIPE */
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  ::sigaction(SIGPIPE, &sa, 0);
#endif

  switch (m_serviceState) {
    case CREATE_JUST: {
      LOG_INFO_NEW("the consumer [{}] start beginning.", getGroupName());

      m_serviceState = START_FAILED;

      // data
      checkConfig();

      copySubscription();

      if (messageModel() == CLUSTERING) {
        changeInstanceNameToPID();
      }

      // ensure m_clientFactory
      MQClient::start();

      // reset rebalance
      m_rebalanceImpl->setConsumerGroup(getGroupName());
      m_rebalanceImpl->setMessageModel(getMessageModel());
      m_rebalanceImpl->setAllocateMQStrategy(getAllocateMQStrategy());
      m_rebalanceImpl->setMQClientFactory(m_clientInstance.get());

      m_pullAPIWrapper.reset(new PullAPIWrapper(m_clientInstance.get(), getGroupName()));

      switch (getMessageModel()) {
        case BROADCASTING:
          m_offsetStore.reset(new LocalFileOffsetStore(m_clientInstance.get(), getGroupName()));
          break;
        case CLUSTERING:
          m_offsetStore.reset(new RemoteBrokerOffsetStore(m_clientInstance.get(), getGroupName()));
          break;
      }
      m_offsetStore->load();

      // checkConfig() guarantee m_pMessageListener is not nullptr
      if (m_messageListener->getMessageListenerType() == messageListenerOrderly) {
        LOG_INFO_NEW("start orderly consume service: {}", getGroupName());
        m_consumeOrderly = true;
        m_consumerService.reset(new ConsumeMessageOrderlyService(this, m_consumeThreadNum, m_messageListener));
      } else {
        // for backward compatible, defaultly and concurrently listeners are allocating
        // ConsumeMessageConcurrentlyService
        LOG_INFO_NEW("start concurrently consume service: {}", getGroupName());
        m_consumeOrderly = false;
        m_consumerService.reset(new ConsumeMessageConcurrentlyService(this, m_consumeThreadNum, m_messageListener));
      }
      m_consumerService->start();

      // register consumer
      bool registerOK = m_clientInstance->registerConsumer(getGroupName(), this);
      if (!registerOK) {
        m_serviceState = CREATE_JUST;
        m_consumerService->shutdown();
        THROW_MQEXCEPTION(
            MQClientException,
            "The cousumer group[" + getGroupName() + "] has been created before, specify another name please.", -1);
      }

      m_clientInstance->start();
      LOG_INFO_NEW("the consumer [{}] start OK", getGroupName());
      m_serviceState = RUNNING;
      break;
    }
    case RUNNING:
    case START_FAILED:
    case SHUTDOWN_ALREADY:
      break;
    default:
      break;
  }

  updateTopicSubscribeInfoWhenSubscriptionChanged();
  m_clientInstance->sendHeartbeatToAllBrokerWithLock();
  m_clientInstance->rebalanceImmediately();
}

void DefaultMQPushConsumer::shutdown() {
  switch (m_serviceState) {
    case RUNNING: {
      m_consumerService->shutdown();
      persistConsumerOffset();
      m_clientInstance->unregisterConsumer(getGroupName());
      m_clientInstance->shutdown();
      LOG_INFO_NEW("the consumer [{}] shutdown OK", getGroupName());
      m_rebalanceImpl->destroy();
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

void DefaultMQPushConsumer::registerMessageListener(MQMessageListener* messageListener) {
  if (nullptr != messageListener) {
    m_messageListener = messageListener;
  }
}

void DefaultMQPushConsumer::registerMessageListener(MessageListenerConcurrently* messageListener) {
  registerMessageListener((MQMessageListener*)messageListener);
}

void DefaultMQPushConsumer::registerMessageListener(MessageListenerOrderly* messageListener) {
  registerMessageListener((MQMessageListener*)messageListener);
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

  if (m_messageListener == nullptr) {
    THROW_MQEXCEPTION(MQClientException, "messageListener is null ", -1);
  }
}

void DefaultMQPushConsumer::copySubscription() {
  for (const auto& it : m_subTopics) {
    LOG_INFO_NEW("buildSubscriptionData: {}, {}", it.first, it.second);
    SubscriptionDataPtr subscriptionData = FilterAPI::buildSubscriptionData(it.first, it.second);
    m_rebalanceImpl->setSubscriptionData(it.first, subscriptionData);
  }

  switch (getMessageModel()) {
    case BROADCASTING:
      break;
    case CLUSTERING: {
      // auto subscript retry topic
      std::string retryTopic = UtilAll::getRetryTopic(getGroupName());
      SubscriptionDataPtr subscriptionData = FilterAPI::buildSubscriptionData(retryTopic, SUB_ALL);
      m_rebalanceImpl->setSubscriptionData(retryTopic, subscriptionData);
      break;
    }
    default:
      break;
  }
}

void DefaultMQPushConsumer::updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info) {
  m_rebalanceImpl->setTopicSubscribeInfo(topic, info);
}

void DefaultMQPushConsumer::updateTopicSubscribeInfoWhenSubscriptionChanged() {
  auto& subTable = m_rebalanceImpl->getSubscriptionInner();
  for (const auto& sub : subTable) {
    bool ret = m_clientInstance->updateTopicRouteInfoFromNameServer(sub.first);
    if (!ret) {
      LOG_WARN_NEW("The topic:[{}] not exist", sub.first);
    }
  }
}

std::string DefaultMQPushConsumer::groupName() const {
  return getGroupName();
}

MessageModel DefaultMQPushConsumer::messageModel() const {
  return getMessageModel();
};

ConsumeType DefaultMQPushConsumer::consumeType() const {
  return CONSUME_PASSIVELY;
}

ConsumeFromWhere DefaultMQPushConsumer::consumeFromWhere() const {
  return getConsumeFromWhere();
}

std::vector<SubscriptionData> DefaultMQPushConsumer::subscriptions() const {
  std::vector<SubscriptionData> result;
  auto& subTable = m_rebalanceImpl->getSubscriptionInner();
  for (const auto& it : subTable) {
    result.push_back(*(it.second));
  }
  return result;
}

void DefaultMQPushConsumer::suspend() {
  m_pause = true;
  LOG_INFO_NEW("suspend this consumer, {}", getGroupName());
}

void DefaultMQPushConsumer::resume() {
  m_pause = false;
  doRebalance();
  LOG_INFO_NEW("resume this consumer, {}", getGroupName());
}

bool DefaultMQPushConsumer::isPause() {
  return m_pause;
}

void DefaultMQPushConsumer::setPause(bool pause) {
  m_pause = pause;
}

void DefaultMQPushConsumer::updateConsumeOffset(const MQMessageQueue& mq, int64_t offset) {
  if (offset >= 0) {
    m_offsetStore->updateOffset(mq, offset, false);
  } else {
    LOG_ERROR_NEW("updateConsumeOffset of mq:{} error", mq.toString());
  }
}

void DefaultMQPushConsumer::correctTagsOffset(PullRequestPtr pullRequest) {
  if (0L == pullRequest->getProcessQueue()->getCacheMsgCount()) {
    m_offsetStore->updateOffset(pullRequest->getMessageQueue(), pullRequest->getNextOffset(), true);
  }
}

void DefaultMQPushConsumer::executePullRequestLater(PullRequestPtr pullRequest, long timeDelay) {
  m_clientInstance->getPullMessageService()->executePullRequestLater(pullRequest, timeDelay);
}

void DefaultMQPushConsumer::executePullRequestImmediately(PullRequestPtr pullRequest) {
  m_clientInstance->getPullMessageService()->executePullRequestImmediately(pullRequest);
}

void DefaultMQPushConsumer::executeTaskLater(const handler_type& task, long timeDelay) {
  m_clientInstance->getPullMessageService()->executeTaskLater(task, timeDelay);
}

void DefaultMQPushConsumer::pullMessage(PullRequestPtr pullRequest) {
  if (nullptr == pullRequest) {
    LOG_ERROR("PullRequest is NULL, return");
    return;
  }

  auto processQueue = pullRequest->getProcessQueue();
  if (processQueue->isDropped()) {
    LOG_WARN_NEW("the pull request[{}] is dropped.", pullRequest->toString());
    return;
  }

  processQueue->setLastPullTimestamp(UtilAll::currentTimeMillis());

  int cachedMessageCount = processQueue->getCacheMsgCount();
  if (cachedMessageCount > m_maxMsgCacheSize) {
    // too many message in cache, wait to process
    executePullRequestLater(pullRequest, 1000);
    return;
  }

  if (isConsumeOrderly()) {
    if (processQueue->isLocked()) {
      if (!pullRequest->isLockedFirst()) {
        const auto offset = m_rebalanceImpl->computePullFromWhere(pullRequest->getMessageQueue());
        bool brokerBusy = offset < pullRequest->getNextOffset();
        LOG_INFO_NEW(
            "the first time to pull message, so fix offset from broker. pullRequest: {} NewOffset: {} brokerBusy: {}",
            pullRequest->toString(), offset, UtilAll::to_string(brokerBusy));
        if (brokerBusy) {
          LOG_INFO_NEW(
              "[NOTIFYME] the first time to pull message, but pull request offset larger than broker consume offset. "
              "pullRequest: {} NewOffset: {}",
              pullRequest->toString(), offset);
        }

        pullRequest->setLockedFirst(true);
        pullRequest->setNextOffset(offset);
      }
    } else {
      executePullRequestLater(pullRequest, 3000);
      LOG_INFO_NEW("pull message later because not locked in broker, {}", pullRequest->toString());
      return;
    }
  }

  const auto& messageQueue = pullRequest->getMessageQueue();
  SubscriptionDataPtr subscriptionData = m_rebalanceImpl->getSubscriptionData(messageQueue.getTopic());
  if (nullptr == subscriptionData) {
    executePullRequestLater(pullRequest, 3000);
    LOG_WARN_NEW("find the consumer's subscription failed, {}", pullRequest->toString());
    return;
  }

  bool commitOffsetEnable = false;
  int64_t commitOffsetValue = 0;
  if (CLUSTERING == getMessageModel()) {
    commitOffsetValue = m_offsetStore->readOffset(messageQueue, READ_FROM_MEMORY);
    if (commitOffsetValue > 0) {
      commitOffsetEnable = true;
    }
  }

  std::string subExpression = subscriptionData->getSubString();

  int sysFlag = PullSysFlag::buildSysFlag(commitOffsetEnable,      // commitOffset
                                          true,                    // suspend
                                          !subExpression.empty(),  // subscription
                                          false);                  // class filter

  try {
    auto* pCallback = new AsyncPullCallback(shared_from_this(), pullRequest, subscriptionData);
    m_pullAPIWrapper->pullKernelImpl(messageQueue,                       // 1
                                     subExpression,                      // 2
                                     subscriptionData->getSubVersion(),  // 3
                                     pullRequest->getNextOffset(),       // 4
                                     32,                                 // 5
                                     sysFlag,                            // 6
                                     commitOffsetValue,                  // 7
                                     1000 * 15,                          // 8
                                     m_asyncPullTimeout,                 // 9
                                     ComMode_ASYNC,                      // 10
                                     pCallback);                         // 11
  } catch (MQException& e) {
    LOG_ERROR_NEW("pullKernelImpl exception: {}", e.what());
    executePullRequestLater(pullRequest, 3000);
  }
}

void DefaultMQPushConsumer::resetRetryTopic(std::vector<MQMessageExtPtr2>& msgs, const std::string& consumerGroup) {
  std::string groupTopic = UtilAll::getRetryTopic(consumerGroup);
  for (auto& msg : msgs) {
    std::string retryTopic = msg->getProperty(MQMessageConst::PROPERTY_RETRY_TOPIC);
    if (!retryTopic.empty() && groupTopic == msg->getTopic()) {
      msg->setTopic(retryTopic);
    }
  }
}

ConsumerRunningInfo* DefaultMQPushConsumer::consumerRunningInfo() {
  auto* info = new ConsumerRunningInfo();

  info->setProperty(ConsumerRunningInfo::PROP_CONSUME_ORDERLY, UtilAll::to_string(m_consumeOrderly));
  info->setProperty(ConsumerRunningInfo::PROP_THREADPOOL_CORE_SIZE, UtilAll::to_string(m_consumeThreadNum));
  info->setProperty(ConsumerRunningInfo::PROP_CONSUMER_START_TIMESTAMP, UtilAll::to_string(m_startTime));

  auto subSet = subscriptions();
  info->setSubscriptionSet(subSet);

  auto processQueueTable = m_rebalanceImpl->getProcessQueueTable();

  for (const auto& it : processQueueTable) {
    const auto& mq = it.first;
    const auto& pq = it.second;

    ProcessQueueInfo pqinfo;
    pqinfo.setCommitOffset(m_offsetStore->readOffset(mq, MEMORY_FIRST_THEN_STORE));
    pq->fillProcessQueueInfo(pqinfo);
    info->setMqTable(mq, pqinfo);
  }

  return info;
}

}  // namespace rocketmq
