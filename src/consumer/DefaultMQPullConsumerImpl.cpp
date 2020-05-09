// /*
//  * Licensed to the Apache Software Foundation (ASF) under one or more
//  * contributor license agreements.  See the NOTICE file distributed with
//  * this work for additional information regarding copyright ownership.
//  * The ASF licenses this file to You under the Apache License, Version 2.0
//  * (the "License"); you may not use this file except in compliance with
//  * the License.  You may obtain a copy of the License at
//  *
//  *     http://www.apache.org/licenses/LICENSE-2.0
//  *
//  * Unless required by applicable law or agreed to in writing, software
//  * distributed under the License is distributed on an "AS IS" BASIS,
//  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  * See the License for the specific language governing permissions and
//  * limitations under the License.
//  */
// #include "DefaultMQPullConsumerImpl.h"

// #ifndef WIN32
// #include <signal.h>
// #endif

// #include "AllocateMQAveragely.h"
// #include "CommunicationMode.h"
// #include "FilterAPI.h"
// #include "Logging.h"
// #include "MQAdminImpl.h"
// #include "MQClientAPIImpl.h"
// #include "MQClientInstance.h"
// #include "MQClientManager.h"
// #include "MQProtos.h"
// #include "OffsetStore.h"
// #include "PullAPIWrapper.h"
// #include "PullSysFlag.h"
// #include "RebalanceImpl.h"
// #include "Validators.h"

// namespace rocketmq {

// DefaultMQPullConsumer::DefaultMQPullConsumer(const string& groupname) : DefaultMQPullConsumer(groupname, nullptr) {}

// DefaultMQPullConsumer::DefaultMQPullConsumer(const string& groupname, RPCHookPtr rpcHook)
//     : MQClient(rpcHook),
//       m_rebalanceImpl(new RebalancePullImpl(this)),
//       m_pullAPIWrapper(nullptr),
//       m_offsetStore(nullptr),
//       m_messageQueueListener(nullptr) {
//   // set default group name
//   if (groupname.empty()) {
//     setGroupName(DEFAULT_CONSUMER_GROUP);
//   } else {
//     setGroupName(groupname);
//   }
// }

// DefaultMQPullConsumer::~DefaultMQPullConsumer() = default;

// void DefaultMQPullConsumer::start() {
// #ifndef WIN32
//   /* Ignore the SIGPIPE */
//   struct sigaction sa;
//   memset(&sa, 0, sizeof(struct sigaction));
//   sa.sa_handler = SIG_IGN;
//   sa.sa_flags = 0;
//   ::sigaction(SIGPIPE, &sa, 0);
// #endif
//   switch (m_serviceState) {
//     case CREATE_JUST: {
//       m_serviceState = START_FAILED;

//       // data
//       checkConfig();

//       copySubscription();

//       if (getMessageModel() == CLUSTERING) {
//         changeInstanceNameToPID();
//       }

//       MQClient::start();
//       LOG_INFO_NEW("DefaultMQPullConsumer:{} start", getGroupName());

//       // reset rebalance;
//       m_rebalanceImpl->setConsumerGroup(getGroupName());
//       m_rebalanceImpl->setMessageModel(getMessageModel());
//       m_rebalanceImpl->setAllocateMQStrategy(getAllocateMQStrategy());
//       m_rebalanceImpl->setMQClientFactory(m_clientInstance.get());

//       m_pullAPIWrapper.reset(new PullAPIWrapper(m_clientInstance.get(), getGroupName()));

//       // msg model
//       switch (getMessageModel()) {
//         case BROADCASTING:
//           m_offsetStore.reset(new LocalFileOffsetStore(m_clientInstance.get(), getGroupName()));
//           break;
//         case CLUSTERING:
//           m_offsetStore.reset(new RemoteBrokerOffsetStore(m_clientInstance.get(), getGroupName()));
//           break;
//       }
//       m_offsetStore->load();

//       // register consumer
//       bool registerOK = m_clientInstance->registerConsumer(getGroupName(), this);
//       if (!registerOK) {
//         m_serviceState = CREATE_JUST;
//         THROW_MQEXCEPTION(
//             MQClientException,
//             "The cousumer group[" + getGroupName() + "] has been created before, specify another name please.", -1);
//       }

//       m_clientInstance->start();
//       LOG_INFO_NEW("the consumer [{}] start OK", getGroupName());
//       m_serviceState = RUNNING;
//       break;
//     }
//     case RUNNING:
//     case START_FAILED:
//     case SHUTDOWN_ALREADY:
//       break;
//     default:
//       break;
//   }
// }

// void DefaultMQPullConsumer::shutdown() {
//   switch (m_serviceState) {
//     case RUNNING: {
//       LOG_INFO("DefaultMQPullConsumer:%s shutdown", m_groupName.c_str());
//       persistConsumerOffset();
//       m_clientInstance->unregisterConsumer(getGroupName());
//       m_clientInstance->shutdown();
//       m_serviceState = SHUTDOWN_ALREADY;
//       break;
//     }
//     case SHUTDOWN_ALREADY:
//     case CREATE_JUST:
//       break;
//     default:
//       break;
//   }
// }

// bool DefaultMQPullConsumer::sendMessageBack(MQMessageExt& msg, int delayLevel) {
//   return false;
// }

// void DefaultMQPullConsumer::fetchSubscribeMessageQueues(const std::string& topic, std::vector<MQMessageQueue>& mqs) {
//   mqs.clear();
//   try {
//     m_clientInstance->getMQAdminImpl()->fetchSubscribeMessageQueues(topic, mqs);
//   } catch (MQException& e) {
//     LOG_ERROR(e.what());
//   }
// }

// void DefaultMQPullConsumer::updateTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue>& info) {}

// void DefaultMQPullConsumer::registerMessageQueueListener(const std::string& topic, MQueueListener* listener) {
//   m_registerTopics.insert(topic);
//   if (listener != nullptr) {
//     m_messageQueueListener = listener;
//   }
// }

// PullResult DefaultMQPullConsumer::pull(const MQMessageQueue& mq,
//                                        const std::string& subExpression,
//                                        int64_t offset,
//                                        int maxNums) {
//   return pullSyncImpl(mq, subExpression, offset, maxNums, false);
// }

// void DefaultMQPullConsumer::pull(const MQMessageQueue& mq,
//                                  const std::string& subExpression,
//                                  int64_t offset,
//                                  int maxNums,
//                                  PullCallback* pPullCallback) {
//   pullAsyncImpl(mq, subExpression, offset, maxNums, false, pPullCallback);
// }

// PullResult DefaultMQPullConsumer::pullBlockIfNotFound(const MQMessageQueue& mq,
//                                                       const std::string& subExpression,
//                                                       int64_t offset,
//                                                       int maxNums) {
//   return pullSyncImpl(mq, subExpression, offset, maxNums, true);
// }

// void DefaultMQPullConsumer::pullBlockIfNotFound(const MQMessageQueue& mq,
//                                                 const std::string& subExpression,
//                                                 int64_t offset,
//                                                 int maxNums,
//                                                 PullCallback* pPullCallback) {
//   pullAsyncImpl(mq, subExpression, offset, maxNums, true, pPullCallback);
// }

// PullResult DefaultMQPullConsumer::pullSyncImpl(const MQMessageQueue& mq,
//                                                const std::string& subExpression,
//                                                int64_t offset,
//                                                int maxNums,
//                                                bool block) {
//   if (offset < 0)
//     THROW_MQEXCEPTION(MQClientException, "offset < 0", -1);

//   if (maxNums <= 0)
//     THROW_MQEXCEPTION(MQClientException, "maxNums <= 0", -1);

//   // auto subscript, all sub
//   subscriptionAutomatically(mq.getTopic());

//   int sysFlag = PullSysFlag::buildSysFlag(false, block, true, false);

//   // this sub
//   std::unique_ptr<SubscriptionData> pSData(FilterAPI::buildSubscriptionData(mq.getTopic(), subExpression));

//   int timeoutMillis = block ? 1000 * 30 : 1000 * 10;

//   try {
//     std::unique_ptr<PullResult> pullResult(m_pullAPIWrapper->pullKernelImpl(mq,                      // 1
//                                                                             pSData->getSubString(),  // 2
//                                                                             0L,                      // 3
//                                                                             offset,                  // 4
//                                                                             maxNums,                 // 5
//                                                                             sysFlag,                 // 6
//                                                                             0,                       // 7
//                                                                             1000 * 20,               // 8
//                                                                             timeoutMillis,           // 9
//                                                                             ComMode_SYNC,            // 10
//                                                                             nullptr));               // callback
//     assert(pullResult != nullptr);
//     return m_pullAPIWrapper->processPullResult(mq, *pullResult, pSData.get());
//   } catch (MQException& e) {
//     LOG_ERROR(e.what());
//   }
//   return PullResult(BROKER_TIMEOUT);
// }

// void DefaultMQPullConsumer::pullAsyncImpl(const MQMessageQueue& mq,
//                                           const std::string& subExpression,
//                                           int64_t offset,
//                                           int maxNums,
//                                           bool block,
//                                           PullCallback* pPullCallback) {
//   if (offset < 0)
//     THROW_MQEXCEPTION(MQClientException, "offset < 0", -1);

//   if (maxNums <= 0)
//     THROW_MQEXCEPTION(MQClientException, "maxNums <= 0", -1);

//   if (!pPullCallback)
//     THROW_MQEXCEPTION(MQClientException, "pPullCallback is null", -1);

//   // auto subscript, all sub
//   subscriptionAutomatically(mq.getTopic());

//   int sysFlag = PullSysFlag::buildSysFlag(false, block, true, false);

//   // this sub
//   std::unique_ptr<SubscriptionData> pSData(FilterAPI::buildSubscriptionData(mq.getTopic(), subExpression));

//   int timeoutMillis = block ? 1000 * 30 : 1000 * 10;

//   try {
//     std::unique_ptr<PullResult> pullResult(m_pullAPIWrapper->pullKernelImpl(mq,                      // 1
//                                                                             pSData->getSubString(),  // 2
//                                                                             0L,                      // 3
//                                                                             offset,                  // 4
//                                                                             maxNums,                 // 5
//                                                                             sysFlag,                 // 6
//                                                                             0,                       // 7
//                                                                             1000 * 20,               // 8
//                                                                             timeoutMillis,           // 9
//                                                                             ComMode_ASYNC,           // 10
//                                                                             pPullCallback));
//   } catch (MQException& e) {
//     LOG_ERROR(e.what());
//   }
// }

// void DefaultMQPullConsumer::subscriptionAutomatically(const std::string& topic) {
//   SubscriptionDataPtr pSdata = m_rebalanceImpl->getSubscriptionData(topic);
//   if (pSdata == nullptr) {
//     std::unique_ptr<SubscriptionData> subscriptionData(FilterAPI::buildSubscriptionData(topic, SUB_ALL));
//     m_rebalanceImpl->setSubscriptionData(topic, subscriptionData.release());
//   }
// }

// void DefaultMQPullConsumer::updateConsumeOffset(const MQMessageQueue& mq, int64_t offset) {
//   m_offsetStore->updateOffset(mq, offset, false);
// }

// void DefaultMQPullConsumer::removeConsumeOffset(const MQMessageQueue& mq) {
//   m_offsetStore->removeOffset(mq);
// }

// int64_t DefaultMQPullConsumer::fetchConsumeOffset(const MQMessageQueue& mq, bool fromStore) {
//   return m_offsetStore->readOffset(mq, fromStore ? READ_FROM_STORE : MEMORY_FIRST_THEN_STORE);
// }

// void DefaultMQPullConsumer::persistConsumerOffset() {
//   /*As do not execute rebalance for pullConsumer now, requestTable is always
//   empty
//   map<MQMessageQueue, PullRequest*> requestTable =
//   m_pRebalance->getPullRequestTable();
//   map<MQMessageQueue, PullRequest*>::iterator it = requestTable.begin();
//   vector<MQMessageQueue> mqs;
//   for (; it != requestTable.end(); ++it)
//   {
//       if (it->second)
//       {
//           mqs.push_back(it->first);
//       }
//   }
//   m_pOffsetStore->persistAll(mqs);*/
// }

// void DefaultMQPullConsumer::persistConsumerOffset4PullConsumer(const MQMessageQueue& mq) {
//   if (isServiceStateOk()) {
//     m_offsetStore->persist(mq);
//   }
// }

// void DefaultMQPullConsumer::fetchMessageQueuesInBalance(const std::string& topic, std::vector<MQMessageQueue>& mqs)
// {}

// void DefaultMQPullConsumer::checkConfig() {
//   string groupname = getGroupName();
//   // check consumerGroup
//   Validators::checkGroup(groupname);

//   // consumerGroup
//   if (!groupname.compare(DEFAULT_CONSUMER_GROUP)) {
//     THROW_MQEXCEPTION(MQClientException, "consumerGroup can not equal DEFAULT_CONSUMER", -1);
//   }

//   if (getMessageModel() != BROADCASTING && getMessageModel() != CLUSTERING) {
//     THROW_MQEXCEPTION(MQClientException, "messageModel is valid ", -1);
//   }
// }

// void DefaultMQPullConsumer::doRebalance() {}

// void DefaultMQPullConsumer::copySubscription() {
//   std::set<string>::iterator it = m_registerTopics.begin();
//   for (; it != m_registerTopics.end(); ++it) {
//     std::unique_ptr<SubscriptionData> subscriptionData(FilterAPI::buildSubscriptionData((*it), SUB_ALL));
//     m_rebalanceImpl->setSubscriptionData((*it), subscriptionData.release());
//   }
// }

// std::string DefaultMQPullConsumer::groupName() const {
//   return getGroupName();
// }

// MessageModel DefaultMQPullConsumer::messageModel() const {
//   return getMessageModel();
// }

// ConsumeType DefaultMQPullConsumer::consumeType() const {
//   return CONSUME_ACTIVELY;
// }

// ConsumeFromWhere DefaultMQPullConsumer::consumeFromWhere() const {
//   return CONSUME_FROM_LAST_OFFSET;
// }

// std::vector<SubscriptionData> DefaultMQPullConsumer::subscriptions() const {
//   std::vector<SubscriptionData> result;
//   std::set<string>::iterator it = m_registerTopics.begin();
//   for (; it != m_registerTopics.end(); ++it) {
//     SubscriptionData ms(*it, SUB_ALL);
//     result.push_back(ms);
//   }
//   return result;
// }

// }  // namespace rocketmq
