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
#include "MQClientFactory.h"

#include "ConsumerRunningInfo.h"
#include "Logging.h"
#include "MQClientManager.h"
#include "PullRequest.h"
#include "Rebalance.h"
#include "TopicPublishInfo.h"
#include "TransactionMQProducer.h"

#define MAX_BUFF_SIZE 8192
#define SAFE_BUFF_SIZE 7936  // 8192 - 256 = 7936
#define PROCESS_NAME_BUF_SIZE 256

namespace rocketmq {

MQClientFactory::MQClientFactory(const std::string& clientID,
                                 int pullThreadNum,
                                 uint64_t tcpConnectTimeout,
                                 uint64_t tcpTransportTryLockTimeout,
                                 std::string unitName)
    : m_clientId(clientID), m_scheduledExecutorService(false), m_rebalanceService(false) {
  // default Topic register;
  std::shared_ptr<TopicPublishInfo> pDefaultTopicInfo(new TopicPublishInfo());
  m_topicPublishInfoTable[DEFAULT_TOPIC] = pDefaultTopicInfo;

  m_pClientRemotingProcessor.reset(new ClientRemotingProcessor(this));
  m_pClientAPIImpl.reset(new MQClientAPIImpl(m_clientId, m_pClientRemotingProcessor.get(), pullThreadNum,
                                             tcpConnectTimeout, tcpTransportTryLockTimeout, unitName));
  m_serviceState = CREATE_JUST;
  LOG_DEBUG("MQClientFactory construct");
}

MQClientFactory::~MQClientFactory() {
  LOG_INFO("MQClientFactory:%s destruct", m_clientId.c_str());

  for (TRDMAP::iterator itp = m_topicRouteTable.begin(); itp != m_topicRouteTable.end(); ++itp) {
    delete itp->second;
  }
  m_topicRouteTable.clear();

  m_producerTable.clear();
  m_consumerTable.clear();
  m_brokerAddrTable.clear();
  m_topicPublishInfoTable.clear();

  m_pClientAPIImpl = nullptr;
}

void MQClientFactory::start() {
  switch (m_serviceState) {
    case CREATE_JUST:
      LOG_INFO("MQClientFactory:%s start", m_clientId.c_str());
      m_serviceState = START_FAILED;

      // start various schedule tasks
      startScheduledTask();

      // start rebalance service
      m_rebalanceService.startup();
      m_rebalanceService.schedule(std::bind(&MQClientFactory::timerCB_doRebalance, this), 10, time_unit::seconds);

      m_serviceState = RUNNING;
      break;
    case RUNNING:
    case SHUTDOWN_ALREADY:
    case START_FAILED:
      LOG_INFO("The Factory object:%s start failed with fault state:%d", m_clientId.c_str(), m_serviceState);
      break;
    default:
      break;
  }
}

void MQClientFactory::updateTopicRouteInfo() {
  if ((getConsumerTableSize() == 0) && (getProducerTableSize() == 0)) {
    return;
  }

  std::set<std::string> topicList;

  // Consumer
  getTopicListFromConsumerSubscription(topicList);

  // Producer
  getTopicListFromTopicPublishInfo(topicList);

  // update
  {
    SessionCredentials session_credentials;
    getSessionCredentialsFromOneOfProducerOrConsumer(session_credentials);
    for (const auto& topic : topicList) {
      updateTopicRouteInfoFromNameServer(topic, session_credentials);
    }
  }

  m_scheduledExecutorService.schedule(std::bind(&MQClientFactory::updateTopicRouteInfo, this), 30, time_unit::seconds);
}

TopicRouteData* MQClientFactory::getTopicRouteData(const std::string& topic) {
  std::lock_guard<std::mutex> lock(m_topicRouteTableMutex);
  if (m_topicRouteTable.find(topic) != m_topicRouteTable.end()) {
    return m_topicRouteTable[topic];
  }
  return NULL;
}

void MQClientFactory::addTopicRouteData(const std::string& topic, TopicRouteData* pTopicRouteData) {
  std::lock_guard<std::mutex> lock(m_topicRouteTableMutex);
  if (m_topicRouteTable.find(topic) != m_topicRouteTable.end()) {
    delete m_topicRouteTable[topic];
    m_topicRouteTable.erase(topic);
  }
  m_topicRouteTable.emplace(topic, pTopicRouteData);
}

std::shared_ptr<TopicPublishInfo> MQClientFactory::tryToFindTopicPublishInfo(
    const std::string& topic,
    const SessionCredentials& session_credentials) {
  // add topicPublishInfoLock to avoid con-current excuting updateTopicRouteInfoFromNameServer
  // when producer send msg  before topicRouteInfo was got;
  std::lock_guard<std::mutex> lock(m_topicPublishInfoLock);

  if (!isTopicInfoValidInTable(topic)) {
    updateTopicRouteInfoFromNameServer(topic, session_credentials);
  }

  // if not exsit ,update dafult topic;
  if (!isTopicInfoValidInTable(topic)) {
    LOG_INFO("updateTopicRouteInfoFromNameServer with default");
    updateTopicRouteInfoFromNameServer(topic, session_credentials, true);
  }

  if (!isTopicInfoValidInTable(topic)) {
    LOG_WARN("tryToFindTopicPublishInfo null:%s", topic.c_str());
    std::shared_ptr<TopicPublishInfo> pTopicPublishInfo;
    return pTopicPublishInfo;
  }

  return getTopicPublishInfoFromTable(topic);
}

bool MQClientFactory::updateTopicRouteInfoFromNameServer(const std::string& topic,
                                                         const SessionCredentials& session_credentials,
                                                         bool isDefault /* = false */) {
  std::lock_guard<std::mutex> lock(m_factoryLock);
  std::unique_ptr<TopicRouteData> pTopicRouteData;
  LOG_INFO("updateTopicRouteInfoFromNameServer start:%s", topic.c_str());

  if (isDefault) {
    pTopicRouteData.reset(
        m_pClientAPIImpl->getTopicRouteInfoFromNameServer(DEFAULT_TOPIC, 1000 * 5, session_credentials));
    if (pTopicRouteData != nullptr) {
      auto& queueDatas = pTopicRouteData->getQueueDatas();
      for (auto& qd : queueDatas) {
        int queueNums = std::min(4, qd.readQueueNums);
        qd.readQueueNums = queueNums;
        qd.writeQueueNums = queueNums;
      }
    }
    LOG_DEBUG("getTopicRouteInfoFromNameServer is null for topic :%s", topic.c_str());
  } else {
    pTopicRouteData.reset(m_pClientAPIImpl->getTopicRouteInfoFromNameServer(topic, 1000 * 5, session_credentials));
  }

  if (pTopicRouteData != nullptr) {
    LOG_INFO("updateTopicRouteInfoFromNameServer has data");
    TopicRouteData* pTemp = getTopicRouteData(topic);
    bool changed = true;
    if (pTemp != NULL) {
      changed = !(*pTemp == *pTopicRouteData);
    }

    // update subscribe info
    if (getConsumerTableSize() > 0) {
      std::vector<MQMessageQueue> mqs;
      topicRouteData2TopicSubscribeInfo(topic, pTopicRouteData.get(), mqs);
      updateConsumerSubscribeTopicInfo(topic, mqs);
    }

    if (changed) {
      LOG_INFO("updateTopicRouteInfoFromNameServer changed:%s", topic.c_str());

      // update broker addr
      auto& brokerDatas = pTopicRouteData->getBrokerDatas();
      for (auto& bd : brokerDatas) {
        LOG_INFO("updateTopicRouteInfoFromNameServer changed with broker name:%s", bd.brokerName.c_str());
        addBrokerToAddrMap(bd.brokerName, bd.brokerAddrs);
      }

      // update publish info
      {
        std::shared_ptr<TopicPublishInfo> publishInfo(topicRouteData2TopicPublishInfo(topic, pTopicRouteData.get()));
        addTopicInfoToTable(topic, publishInfo);  // erase first, then add
      }

      // update subscribe info
      addTopicRouteData(topic, pTopicRouteData.release());
    }
    LOG_DEBUG("updateTopicRouteInfoFromNameServer end:%s", topic.c_str());
    return true;
  }
  LOG_DEBUG("updateTopicRouteInfoFromNameServer end null:%s", topic.c_str());
  return false;
}

std::shared_ptr<TopicPublishInfo> MQClientFactory::topicRouteData2TopicPublishInfo(const std::string& topic,
                                                                                   TopicRouteData* pRoute) {
  std::shared_ptr<TopicPublishInfo> info(new TopicPublishInfo());
  std::string OrderTopicConf = pRoute->getOrderTopicConf();
  // order msg
  if (!OrderTopicConf.empty()) {
    // "broker-a:8";"broker-b:8"
    std::vector<std::string> brokers;
    UtilAll::Split(brokers, OrderTopicConf, ';');
    for (const auto& broker : brokers) {
      std::vector<std::string> item;
      UtilAll::Split(item, broker, ':');
      int nums = atoi(item[1].c_str());
      for (int i = 0; i < nums; i++) {
        MQMessageQueue mq(topic, item[0], i);
        info->updateMessageQueueList(mq);
      }
    }
  }
  // no order msg
  else {
    std::vector<QueueData>& queueDatas = pRoute->getQueueDatas();
    for (const auto& qd : queueDatas) {
      if (PermName::isWriteable(qd.perm)) {
        string addr = findBrokerAddressInPublish(qd.brokerName);
        if (addr.empty()) {
          continue;
        }
        for (int i = 0; i < qd.writeQueueNums; i++) {
          MQMessageQueue mq(topic, qd.brokerName, i);
          info->updateMessageQueueList(mq);
        }
      }
    }
  }
  return info;
}

void MQClientFactory::topicRouteData2TopicSubscribeInfo(const std::string& topic,
                                                        TopicRouteData* pRoute,
                                                        std::vector<MQMessageQueue>& mqs) {
  mqs.clear();
  std::vector<QueueData>& queueDatas = pRoute->getQueueDatas();
  for (const auto& qd : queueDatas) {
    if (PermName::isReadable(qd.perm)) {
      for (int i = 0; i < qd.readQueueNums; i++) {
        MQMessageQueue mq(topic, qd.brokerName, i);
        mqs.push_back(mq);
      }
    }
  }
}

void MQClientFactory::shutdown() {
  if (getConsumerTableSize() != 0)
    return;

  if (getProducerTableSize() != 0)
    return;

  switch (m_serviceState) {
    case RUNNING: {
      m_rebalanceService.shutdown();

      m_scheduledExecutorService.shutdown();

      // Note: stop all TcpTransport Threads and release all responseFuture conditions
      m_pClientAPIImpl->stopAllTcpTransportThread();

      m_serviceState = SHUTDOWN_ALREADY;
      LOG_INFO("MQClientFactory:%s shutdown", m_clientId.c_str());
      break;
    }
    case SHUTDOWN_ALREADY:
    case CREATE_JUST:
      break;
    default:
      break;
  }

  MQClientManager::getInstance()->removeClientFactory(m_clientId);
}

bool MQClientFactory::registerProducer(MQProducer* pProducer) {
  string groupName = pProducer->getGroupName();
  string namesrvaddr = pProducer->getNamesrvAddr();
  if (groupName.empty() || namesrvaddr.empty()) {
    return false;
  }

  if (!addProducerToTable(groupName, pProducer)) {
    return false;
  }

  LOG_DEBUG("registerProducer success:%s", groupName.c_str());

  // set nameserver;
  m_pClientAPIImpl->updateNameServerAddr(namesrvaddr);
  LOG_INFO("user specfied name server address: %s", namesrvaddr.c_str());

  return true;
}

void MQClientFactory::unregisterProducer(MQProducer* pProducer) {
  string groupName = pProducer->getGroupName();
  unregisterClient(groupName, "", pProducer->getSessionCredentials());

  eraseProducerFromTable(groupName);
}

bool MQClientFactory::registerConsumer(MQConsumer* pConsumer) {
  string groupName = pConsumer->getGroupName();
  string namesrvaddr = pConsumer->getNamesrvAddr();
  if (groupName.empty() || namesrvaddr.empty()) {
    return false;
  }

  if (!addConsumerToTable(groupName, pConsumer)) {
    return false;
  }

  LOG_DEBUG("registerConsumer success:%s", groupName.c_str());

  // set nameserver;
  m_pClientAPIImpl->updateNameServerAddr(namesrvaddr);
  LOG_INFO("user specfied name server address: %s", namesrvaddr.c_str());

  return true;
}

void MQClientFactory::unregisterConsumer(MQConsumer* pConsumer) {
  string groupName = pConsumer->getGroupName();
  unregisterClient("", groupName, pConsumer->getSessionCredentials());

  eraseConsumerFromTable(groupName);
}

MQProducer* MQClientFactory::selectProducer(const std::string& producerName) {
  std::lock_guard<std::mutex> lock(m_producerTableMutex);
  if (m_producerTable.find(producerName) != m_producerTable.end()) {
    return m_producerTable[producerName];
  }
  return NULL;
}

bool MQClientFactory::getSessionCredentialFromProducerTable(SessionCredentials& sessionCredentials) {
  std::lock_guard<std::mutex> lock(m_producerTableMutex);
  for (MQPMAP::iterator it = m_producerTable.begin(); it != m_producerTable.end(); ++it) {
    if (it->second)
      sessionCredentials = it->second->getSessionCredentials();
  }

  if (sessionCredentials.isValid())
    return true;

  return false;
}

bool MQClientFactory::addProducerToTable(const std::string& producerName, MQProducer* pMQProducer) {
  std::lock_guard<std::mutex> lock(m_producerTableMutex);
  if (m_producerTable.find(producerName) != m_producerTable.end())
    return false;
  m_producerTable[producerName] = pMQProducer;
  return true;
}

void MQClientFactory::eraseProducerFromTable(const std::string& producerName) {
  std::lock_guard<std::mutex> lock(m_producerTableMutex);
  if (m_producerTable.find(producerName) != m_producerTable.end())
    m_producerTable.erase(producerName);
}

int MQClientFactory::getProducerTableSize() {
  std::lock_guard<std::mutex> lock(m_producerTableMutex);
  return m_producerTable.size();
}

void MQClientFactory::insertProducerInfoToHeartBeatData(HeartbeatData* pHeartbeatData) {
  std::lock_guard<std::mutex> lock(m_producerTableMutex);
  for (MQPMAP::iterator it = m_producerTable.begin(); it != m_producerTable.end(); ++it) {
    ProducerData producerData;
    producerData.groupName = it->first;
    pHeartbeatData->insertDataToProducerDataSet(producerData);
  }
}

MQConsumer* MQClientFactory::selectConsumer(const std::string& group) {
  std::lock_guard<std::mutex> lock(m_consumerTableMutex);
  if (m_consumerTable.find(group) != m_consumerTable.end()) {
    return m_consumerTable[group];
  }
  return NULL;
}

bool MQClientFactory::getSessionCredentialFromConsumerTable(SessionCredentials& sessionCredentials) {
  std::lock_guard<std::mutex> lock(m_consumerTableMutex);
  for (MQCMAP::iterator it = m_consumerTable.begin(); it != m_consumerTable.end(); ++it) {
    if (it->second)
      sessionCredentials = it->second->getSessionCredentials();
  }

  if (sessionCredentials.isValid())
    return true;

  return false;
}

bool MQClientFactory::getSessionCredentialFromConsumer(const std::string& consumerGroup,
                                                       SessionCredentials& sessionCredentials) {
  std::lock_guard<std::mutex> lock(m_consumerTableMutex);
  if (m_consumerTable.find(consumerGroup) != m_consumerTable.end()) {
    sessionCredentials = m_consumerTable[consumerGroup]->getSessionCredentials();
  }

  if (sessionCredentials.isValid())
    return true;

  return false;
}

bool MQClientFactory::addConsumerToTable(const std::string& consumerName, MQConsumer* pMQConsumer) {
  std::lock_guard<std::mutex> lock(m_consumerTableMutex);
  if (m_consumerTable.find(consumerName) != m_consumerTable.end())
    return false;
  m_consumerTable[consumerName] = pMQConsumer;
  return true;
}

void MQClientFactory::eraseConsumerFromTable(const std::string& consumerName) {
  std::lock_guard<std::mutex> lock(m_consumerTableMutex);
  if (m_consumerTable.find(consumerName) != m_consumerTable.end())
    m_consumerTable.erase(consumerName);  // do not need freee pConsumer, as it
                                          // was allocated by user
  else
    LOG_WARN("could not find consumer:%s from table", consumerName.c_str());
}

int MQClientFactory::getConsumerTableSize() {
  std::lock_guard<std::mutex> lock(m_consumerTableMutex);
  return m_consumerTable.size();
}

void MQClientFactory::getTopicListFromConsumerSubscription(std::set<std::string>& topicList) {
  std::lock_guard<std::mutex> lock(m_consumerTableMutex);
  for (auto& it : m_consumerTable) {
    std::vector<SubscriptionData> result;
    it.second->getSubscriptions(result);
    for (const auto& sd : result) {
      topicList.insert(sd.getTopic());
    }
  }
}

void MQClientFactory::updateConsumerSubscribeTopicInfo(const std::string& topic, std::vector<MQMessageQueue> mqs) {
  std::lock_guard<std::mutex> lock(m_consumerTableMutex);
  for (auto& it : m_consumerTable) {
    it.second->updateTopicSubscribeInfo(topic, mqs);
  }
}

void MQClientFactory::insertConsumerInfoToHeartBeatData(HeartbeatData* pHeartbeatData) {
  std::lock_guard<std::mutex> lock(m_consumerTableMutex);
  for (MQCMAP::iterator it = m_consumerTable.begin(); it != m_consumerTable.end(); ++it) {
    MQConsumer* pConsumer = it->second;
    ConsumerData consumerData;
    consumerData.groupName = pConsumer->getGroupName();
    consumerData.consumeType = pConsumer->getConsumeType();
    consumerData.messageModel = pConsumer->getMessageModel();
    consumerData.consumeFromWhere = pConsumer->getConsumeFromWhere();

    // fill data;
    std::vector<SubscriptionData> result;
    pConsumer->getSubscriptions(result);
    consumerData.subscriptionDataSet.swap(result);

    pHeartbeatData->insertDataToConsumerDataSet(consumerData);
  }
}

void MQClientFactory::addTopicInfoToTable(const std::string& topic,
                                          std::shared_ptr<TopicPublishInfo> pTopicPublishInfo) {
  std::lock_guard<std::mutex> lock(m_topicPublishInfoTableMutex);
  if (m_topicPublishInfoTable.find(topic) != m_topicPublishInfoTable.end()) {
    m_topicPublishInfoTable.erase(topic);
  }
  m_topicPublishInfoTable[topic] = pTopicPublishInfo;
}

void MQClientFactory::eraseTopicInfoFromTable(const std::string& topic) {
  std::lock_guard<std::mutex> lock(m_topicPublishInfoTableMutex);
  if (m_topicPublishInfoTable.find(topic) != m_topicPublishInfoTable.end()) {
    m_topicPublishInfoTable.erase(topic);
  }
}

bool MQClientFactory::isTopicInfoValidInTable(const std::string& topic) {
  std::lock_guard<std::mutex> lock(m_topicPublishInfoTableMutex);
  if (m_topicPublishInfoTable.find(topic) != m_topicPublishInfoTable.end()) {
    if (m_topicPublishInfoTable[topic]->ok())
      return true;
  }
  return false;
}

std::shared_ptr<TopicPublishInfo> MQClientFactory::getTopicPublishInfoFromTable(const std::string& topic) {
  std::lock_guard<std::mutex> lock(m_topicPublishInfoTableMutex);
  if (m_topicPublishInfoTable.find(topic) != m_topicPublishInfoTable.end()) {
    return m_topicPublishInfoTable[topic];
  }
  std::shared_ptr<TopicPublishInfo> pTopicPublishInfo;
  return pTopicPublishInfo;
}

void MQClientFactory::getTopicListFromTopicPublishInfo(std::set<std::string>& topicList) {
  std::lock_guard<std::mutex> lock(m_topicPublishInfoTableMutex);
  for (const auto& it : m_topicPublishInfoTable) {
    topicList.insert(it.first);
  }
}

void MQClientFactory::clearBrokerAddrMap() {
  std::lock_guard<std::mutex> lock(m_brokerAddrlock);
  m_brokerAddrTable.clear();
}

void MQClientFactory::addBrokerToAddrMap(const std::string& brokerName, map<int, string>& brokerAddrs) {
  std::lock_guard<std::mutex> lock(m_brokerAddrlock);
  if (m_brokerAddrTable.find(brokerName) != m_brokerAddrTable.end()) {
    m_brokerAddrTable.erase(brokerName);
  }
  m_brokerAddrTable.emplace(brokerName, brokerAddrs);
}

MQClientFactory::BrokerAddrMAP MQClientFactory::getBrokerAddrMap() {
  std::lock_guard<std::mutex> lock(m_brokerAddrlock);
  return m_brokerAddrTable;
}

string MQClientFactory::findBrokerAddressInPublish(const std::string& brokerName) {
  /*reslove the concurrent access m_brokerAddrTable by
  findBrokerAddressInPublish(called by sendKernlImpl) And
  sendHeartbeatToAllBroker, which leads hign RT of sendMsg
  1. change m_brokerAddrTable from hashMap to map;
  2. do not add m_factoryLock here, but copy m_brokerAddrTable,
      this is used to avoid con-current access m_factoryLock by
  findBrokerAddressInPublish(called by sendKernlImpl) And
  updateTopicRouteInfoFromNameServer

   Note: after copying m_brokerAddrTable, updateTopicRouteInfoFromNameServer
  modify m_brokerAddrTable imediatly,
           after 1st send fail, producer will get topicPushlibshInfo again
  before next try, so 2nd try will get correct broker to send ms;
   */
  BrokerAddrMAP brokerTable(getBrokerAddrMap());
  string brokerAddr;
  bool found = false;

  if (brokerTable.find(brokerName) != brokerTable.end()) {
    map<int, string> brokerMap(brokerTable[brokerName]);
    map<int, string>::iterator it1 = brokerMap.find(MASTER_ID);
    if (it1 != brokerMap.end()) {
      brokerAddr = it1->second;
      found = true;
    }
  }

  brokerTable.clear();
  if (found)
    return brokerAddr;

  return "";
}

FindBrokerResult* MQClientFactory::findBrokerAddressInSubscribe(const std::string& brokerName,
                                                                int brokerId,
                                                                bool onlyThisBroker) {
  string brokerAddr;
  bool slave = false;
  bool found = false;
  BrokerAddrMAP brokerTable(getBrokerAddrMap());

  if (brokerTable.find(brokerName) != brokerTable.end()) {
    map<int, string> brokerMap(brokerTable[brokerName]);
    if (!brokerMap.empty()) {
      auto iter = brokerMap.find(brokerId);
      if (iter != brokerMap.end()) {
        brokerAddr = iter->second;
        slave = (brokerId != MASTER_ID);
        found = true;
      } else if (!onlyThisBroker) {  // not only from master
        iter = brokerMap.begin();
        brokerAddr = iter->second;
        slave = iter->first != MASTER_ID;
        found = true;
      }
    }
  }

  brokerTable.clear();

  if (found) {
    return new FindBrokerResult(brokerAddr, slave);
  }

  return nullptr;
}

FindBrokerResult* MQClientFactory::findBrokerAddressInAdmin(const std::string& brokerName) {
  BrokerAddrMAP brokerTable(getBrokerAddrMap());
  bool found = false;
  bool slave = false;
  string brokerAddr;

  if (brokerTable.find(brokerName) != brokerTable.end()) {
    map<int, string> brokerMap(brokerTable[brokerName]);
    map<int, string>::iterator it1 = brokerMap.begin();
    if (it1 != brokerMap.end()) {
      slave = (it1->first != MASTER_ID);
      found = true;
      brokerAddr = it1->second;
    }
  }

  brokerTable.clear();
  if (found)
    return new FindBrokerResult(brokerAddr, slave);

  return NULL;
}

void MQClientFactory::checkTransactionState(const std::string& addr,
                                            const MQMessageExt& messageExt,
                                            const CheckTransactionStateRequestHeader& checkRequestHeader) {
  string group = messageExt.getProperty(MQMessage::PROPERTY_PRODUCER_GROUP);
  if (!group.empty()) {
    MQProducer* producer = selectProducer(group);
    if (producer != nullptr) {
      TransactionMQProducer* transProducer = dynamic_cast<TransactionMQProducer*>(producer);
      if (transProducer != nullptr) {
        transProducer->checkTransactionState(addr, messageExt, checkRequestHeader.m_tranStateTableOffset,
                                             checkRequestHeader.m_commitLogOffset, checkRequestHeader.m_msgId,
                                             checkRequestHeader.m_transactionId, checkRequestHeader.m_offsetMsgId);
      } else {
        LOG_ERROR("checkTransactionState, producer not TransactionMQProducer failed, msg:%s",
                  messageExt.toString().data());
      }
    } else {
      LOG_ERROR("checkTransactionState, pick producer by group[%s] failed, msg:%s", group.data(),
                messageExt.toString().data());
    }
  } else {
    LOG_ERROR("checkTransactionState, pick producer group failed, msg:%s", messageExt.toString().data());
  }
}

MQClientAPIImpl* MQClientFactory::getMQClientAPIImpl() const {
  return m_pClientAPIImpl.get();
}

void MQClientFactory::sendHeartbeatToAllBroker() {
  BrokerAddrMAP brokerTable(getBrokerAddrMap());
  if (brokerTable.size() == 0) {
    LOG_WARN("sendheartbeat brokeradd is empty");
    return;
  }

  std::unique_ptr<HeartbeatData> heartbeatData(prepareHeartbeatData());
  bool producerEmpty = heartbeatData->isProducerDataSetEmpty();
  bool consumerEmpty = heartbeatData->isConsumerDataSetEmpty();
  if (producerEmpty && consumerEmpty) {
    LOG_WARN("sendheartbeat heartbeatData empty");
    brokerTable.clear();
    return;
  }

  SessionCredentials session_credentials;
  getSessionCredentialsFromOneOfProducerOrConsumer(session_credentials);
  for (const auto& it : brokerTable) {
    auto& brokerMap = it.second;
    for (const auto& it1 : brokerMap) {
      const string& addr = it1.second;
      if (consumerEmpty && it1.first != MASTER_ID) {
        continue;
      }

      try {
        m_pClientAPIImpl->sendHearbeat(addr, heartbeatData.get(), session_credentials);
      } catch (MQException& e) {
        LOG_ERROR(e.what());
      }
    }
  }
  brokerTable.clear();
}

void MQClientFactory::persistAllConsumerOffset() {
  {
    std::lock_guard<std::mutex> lock(m_consumerTableMutex);
    if (m_consumerTable.size() > 0) {
      for (auto& it : m_consumerTable) {
        LOG_DEBUG("Client factory start persistAllConsumerOffset");
        it.second->persistConsumerOffset();
      }
    }
  }

  m_rebalanceService.schedule(std::bind(&MQClientFactory::persistAllConsumerOffset, this), 5, time_unit::seconds);
}

HeartbeatData* MQClientFactory::prepareHeartbeatData() {
  HeartbeatData* pHeartbeatData = new HeartbeatData();

  // clientID
  pHeartbeatData->setClientID(m_clientId);

  // Consumer
  insertConsumerInfoToHeartBeatData(pHeartbeatData);

  // Producer
  insertProducerInfoToHeartBeatData(pHeartbeatData);

  return pHeartbeatData;
}

void MQClientFactory::timerCB_sendHeartbeatToAllBroker() {
  sendHeartbeatToAllBroker();

  m_scheduledExecutorService.schedule(std::bind(&MQClientFactory::timerCB_sendHeartbeatToAllBroker, this), 30,
                                      time_unit::seconds);
}

void MQClientFactory::startScheduledTask() {
  m_scheduledExecutorService.startup();

  LOG_INFO("start scheduled task:%s", m_clientId.c_str());

  m_scheduledExecutorService.schedule(std::bind(&MQClientFactory::updateTopicRouteInfo, this), 10,
                                      time_unit::milliseconds);

  m_scheduledExecutorService.schedule(std::bind(&MQClientFactory::timerCB_sendHeartbeatToAllBroker, this), 1000,
                                      time_unit::milliseconds);

  m_scheduledExecutorService.schedule(std::bind(&MQClientFactory::persistAllConsumerOffset, this), 1000 * 10,
                                      time_unit::milliseconds);
}

void MQClientFactory::rebalanceImmediately() {
  m_rebalanceService.schedule(std::bind(&MQClientFactory::doRebalance, this), 0, time_unit::milliseconds);
}

void MQClientFactory::rebalanceByConsumerGroupImmediately(const std::string& consumerGroup) {
  m_rebalanceService.schedule(std::bind(&MQClientFactory::doRebalanceByConsumerGroup, this, consumerGroup), 0,
                              time_unit::milliseconds);
}

void MQClientFactory::timerCB_doRebalance() {
  doRebalance();

  m_rebalanceService.schedule(std::bind(&MQClientFactory::timerCB_doRebalance, this), 10, time_unit::seconds);
}

void MQClientFactory::doRebalance() {
  LOG_INFO("Client factory:%s start dorebalance", m_clientId.c_str());
  if (getConsumerTableSize() > 0) {
    std::lock_guard<std::mutex> lock(m_consumerTableMutex);
    for (auto& it : m_consumerTable) {
      it.second->doRebalance();
    }
  }
  LOG_INFO("Client factory:%s finish dorebalance", m_clientId.c_str());
}

void MQClientFactory::doRebalanceByConsumerGroup(const std::string& consumerGroup) {
  std::lock_guard<std::mutex> lock(m_consumerTableMutex);
  if (m_consumerTable.find(consumerGroup) != m_consumerTable.end()) {
    LOG_INFO("Client factory:%s start dorebalance for consumer:%s", m_clientId.c_str(), consumerGroup.c_str());
    MQConsumer* pMQConsumer = m_consumerTable[consumerGroup];
    pMQConsumer->doRebalance();
  }
}

void MQClientFactory::endTransactionOneway(const MQMessageQueue& mq,
                                           EndTransactionRequestHeader* requestHeader,
                                           const SessionCredentials& sessionCredentials) {
  string brokerAddr = findBrokerAddressInPublish(mq.getBrokerName());
  string remark = "";
  if (!brokerAddr.empty()) {
    try {
      getMQClientAPIImpl()->endTransactionOneway(brokerAddr, requestHeader, remark, sessionCredentials);
    } catch (MQException& e) {
      LOG_ERROR("endTransactionOneway exception:%s", e.what());
      throw e;
    }
  } else {
    THROW_MQEXCEPTION(MQClientException, "The broker[" + mq.getBrokerName() + "] not exist", -1);
  }
}

void MQClientFactory::unregisterClient(const std::string& producerGroup,
                                       const std::string& consumerGroup,
                                       const SessionCredentials& sessionCredentials) {
  BrokerAddrMAP brokerTable(getBrokerAddrMap());
  for (const auto& it : brokerTable) {
    auto& brokerMap = it.second;
    for (const auto& it1 : brokerMap) {
      const string& addr = it1.second;
      m_pClientAPIImpl->unregisterClient(addr, m_clientId, producerGroup, consumerGroup, sessionCredentials);
    }
  }
}

//<!************************************************************************
void MQClientFactory::fetchSubscribeMessageQueues(const std::string& topic,
                                                  std::vector<MQMessageQueue>& mqs,
                                                  const SessionCredentials& sessionCredentials) {
  TopicRouteData* pTopicRouteData = getTopicRouteData(topic);
  if (pTopicRouteData == NULL) {
    updateTopicRouteInfoFromNameServer(topic, sessionCredentials);
    pTopicRouteData = getTopicRouteData(topic);
  }
  if (pTopicRouteData != NULL) {
    topicRouteData2TopicSubscribeInfo(topic, pTopicRouteData, mqs);
    if (mqs.empty()) {
      THROW_MQEXCEPTION(MQClientException, "Can not find Message Queue", -1);
    }
    return;
  }
  THROW_MQEXCEPTION(MQClientException, "Can not find Message Queue", -1);
}

//<!***************************************************************************
void MQClientFactory::createTopic(const std::string& key,
                                  const std::string& newTopic,
                                  int queueNum,
                                  const SessionCredentials& sessionCredentials) {}

int64 MQClientFactory::minOffset(const MQMessageQueue& mq, const SessionCredentials& sessionCredentials) {
  string brokerAddr = findBrokerAddressInPublish(mq.getBrokerName());
  if (brokerAddr.empty()) {
    updateTopicRouteInfoFromNameServer(mq.getTopic(), sessionCredentials);
    brokerAddr = findBrokerAddressInPublish(mq.getBrokerName());
  }

  if (!brokerAddr.empty()) {
    try {
      return m_pClientAPIImpl->getMinOffset(brokerAddr, mq.getTopic(), mq.getQueueId(), 1000 * 3, sessionCredentials);
    } catch (MQException& e) {
      LOG_ERROR(e.what());
    }
  }
  THROW_MQEXCEPTION(MQClientException, "The broker is not exist", -1);
}

int64 MQClientFactory::maxOffset(const MQMessageQueue& mq, const SessionCredentials& sessionCredentials) {
  string brokerAddr = findBrokerAddressInPublish(mq.getBrokerName());
  if (brokerAddr.empty()) {
    updateTopicRouteInfoFromNameServer(mq.getTopic(), sessionCredentials);
    brokerAddr = findBrokerAddressInPublish(mq.getBrokerName());
  }

  if (!brokerAddr.empty()) {
    try {
      return m_pClientAPIImpl->getMaxOffset(brokerAddr, mq.getTopic(), mq.getQueueId(), 1000 * 3, sessionCredentials);
    } catch (MQException& e) {
      THROW_MQEXCEPTION(MQClientException, "Invoke Broker exception", -1);
    }
  }
  THROW_MQEXCEPTION(MQClientException, "The broker is not exist", -1);
}

int64 MQClientFactory::searchOffset(const MQMessageQueue& mq,
                                    int64 timestamp,
                                    const SessionCredentials& sessionCredentials) {
  string brokerAddr = findBrokerAddressInPublish(mq.getBrokerName());
  if (brokerAddr.empty()) {
    updateTopicRouteInfoFromNameServer(mq.getTopic(), sessionCredentials);
    brokerAddr = findBrokerAddressInPublish(mq.getBrokerName());
  }

  if (!brokerAddr.empty()) {
    try {
      return m_pClientAPIImpl->searchOffset(brokerAddr, mq.getTopic(), mq.getQueueId(), timestamp, 1000 * 3,
                                            sessionCredentials);
    } catch (MQException& e) {
      THROW_MQEXCEPTION(MQClientException, "Invoke Broker exception", -1);
    }
  }
  THROW_MQEXCEPTION(MQClientException, "The broker is not exist", -1);
}

MQMessageExt* MQClientFactory::viewMessage(const std::string& msgId, const SessionCredentials& sessionCredentials) {
  try {
    return NULL;
  } catch (MQException& e) {
    THROW_MQEXCEPTION(MQClientException, "message id illegal", -1);
  }
}

int64 MQClientFactory::earliestMsgStoreTime(const MQMessageQueue& mq, const SessionCredentials& sessionCredentials) {
  string brokerAddr = findBrokerAddressInPublish(mq.getBrokerName());
  if (brokerAddr.empty()) {
    updateTopicRouteInfoFromNameServer(mq.getTopic(), sessionCredentials);
    brokerAddr = findBrokerAddressInPublish(mq.getBrokerName());
  }

  if (!brokerAddr.empty()) {
    try {
      return m_pClientAPIImpl->getEarliestMsgStoretime(brokerAddr, mq.getTopic(), mq.getQueueId(), 1000 * 3,
                                                       sessionCredentials);
    } catch (MQException& e) {
      THROW_MQEXCEPTION(MQClientException, "Invoke Broker exception", -1);
    }
  }
  THROW_MQEXCEPTION(MQClientException, "The broker is not exist", -1);
}

QueryResult MQClientFactory::queryMessage(const std::string& topic,
                                          const std::string& key,
                                          int maxNum,
                                          int64 begin,
                                          int64 end,
                                          const SessionCredentials& sessionCredentials) {
  THROW_MQEXCEPTION(MQClientException, "queryMessage", -1);
}

void MQClientFactory::findConsumerIds(const std::string& topic,
                                      const std::string& group,
                                      std::vector<string>& cids,
                                      const SessionCredentials& sessionCredentials) {
  string brokerAddr;
  TopicRouteData* pTopicRouteData = getTopicRouteData(topic);
  if (pTopicRouteData == NULL) {
    updateTopicRouteInfoFromNameServer(topic, sessionCredentials);
    pTopicRouteData = getTopicRouteData(topic);
  }
  if (pTopicRouteData != NULL) {
    brokerAddr = pTopicRouteData->selectBrokerAddr();
  }

  if (!brokerAddr.empty()) {
    try {
      LOG_INFO("getConsumerIdList from broker:%s", brokerAddr.c_str());
      return m_pClientAPIImpl->getConsumerIdListByGroup(brokerAddr, group, cids, 5000, sessionCredentials);
    } catch (MQException& e) {
      LOG_ERROR(e.what());
    }
  }
}

void MQClientFactory::resetOffset(const std::string& group,
                                  const std::string& topic,
                                  const map<MQMessageQueue, int64>& offsetTable) {
  MQConsumer* pConsumer = selectConsumer(group);
  if (pConsumer) {
    for (const auto& it : offsetTable) {
      const MQMessageQueue& mq = it.first;
      std::shared_ptr<PullRequest> pullreq = pConsumer->getRebalance()->getPullRequest(mq);
      if (pullreq) {
        pullreq->setDroped(true);
        LOG_INFO("resetOffset setDroped for mq:%s", mq.toString().data());
        pullreq->clearAllMsgs();
        pullreq->updateQueueMaxOffset(it.second);
      } else {
        LOG_ERROR("no corresponding pullRequest found for topic:%s", topic.c_str());
      }
    }

    for (const auto& it : offsetTable) {
      const MQMessageQueue& mq = it.first;
      if (topic == mq.getTopic()) {
        LOG_INFO("offset sets to:%lld", it.second);
        pConsumer->updateConsumeOffset(mq, it.second);
      }
    }
    pConsumer->persistConsumerOffsetByResetOffset();

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    for (const auto& it : offsetTable) {
      const MQMessageQueue& mq = it.first;
      if (topic == mq.getTopic()) {
        LOG_DEBUG("resetOffset sets to:%lld for mq:%s", it.second, mq.toString().c_str());
        pConsumer->updateConsumeOffset(mq, it.second);
      }
    }
    pConsumer->persistConsumerOffsetByResetOffset();

    for (const auto& it : offsetTable) {
      const MQMessageQueue& mq = it.first;
      if (topic == mq.getTopic()) {
        pConsumer->removeConsumeOffset(mq);
      }
    }

    // do call pConsumer->doRebalance directly here, as it is conflict with timerCB_doRebalance;
    doRebalanceByConsumerGroup(pConsumer->getGroupName());
  } else {
    LOG_ERROR("no corresponding consumer found for group:%s", group.c_str());
  }
}

ConsumerRunningInfo* MQClientFactory::consumerRunningInfo(const std::string& consumerGroup) {
  MQConsumer* pConsumer = selectConsumer(consumerGroup);
  if (pConsumer) {
    ConsumerRunningInfo* runningInfo = pConsumer->getConsumerRunningInfo();
    if (runningInfo) {
      runningInfo->setProperty(ConsumerRunningInfo::PROP_NAMESERVER_ADDR, pConsumer->getNamesrvAddr());
      if (pConsumer->getConsumeType() == CONSUME_PASSIVELY) {
        runningInfo->setProperty(ConsumerRunningInfo::PROP_CONSUME_TYPE, "CONSUME_PASSIVELY");
      } else {
        runningInfo->setProperty(ConsumerRunningInfo::PROP_CONSUME_TYPE, "CONSUME_ACTIVELY");
      }
      runningInfo->setProperty(ConsumerRunningInfo::PROP_CLIENT_VERSION, "V3_1_8");  // MQVersion::s_CurrentVersion ));

      return runningInfo;
    }
  }

  LOG_ERROR("no corresponding consumer found for group:%s", consumerGroup.c_str());
  return NULL;
}

void MQClientFactory::getSessionCredentialsFromOneOfProducerOrConsumer(SessionCredentials& session_credentials) {
  // Note: on the same MQClientFactory, all producers and consumers used the same sessionCredentials,
  // So only need get sessionCredentials from the first one producer or consumer now.
  // this function was only used by updateTopicRouteInfo() and sendHeartbeatToAllBrokers() now.
  // if this strategy was changed in future, need get sessionCredentials for each producer and consumer.
  getSessionCredentialFromProducerTable(session_credentials);
  if (!session_credentials.isValid())
    getSessionCredentialFromConsumerTable(session_credentials);

  if (!session_credentials.isValid()) {
    LOG_ERROR(
        "updateTopicRouteInfo: didn't get the session_credentials from any "
        "producers and consumers, please re-intialize it");
  }
}

}  // namespace rocketmq
