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
#ifndef __MQCLIENTFACTORY_H__
#define __MQCLIENTFACTORY_H__

#include <memory>
#include <mutex>

#include "FindBrokerResult.h"
#include "MQClientAPIImpl.h"
#include "MQClientException.h"
#include "MQConsumer.h"
#include "MQDecoder.h"
#include "MQMessageQueue.h"
#include "MQProducer.h"
#include "PermName.h"
#include "QueryResult.h"
#include "ServiceState.h"
#include "SocketUtil.h"
#include "TopicConfig.h"
#include "TopicRouteData.h"

namespace rocketmq {

class TopicPublishInfo;

class MQClientFactory {
 public:
  MQClientFactory(const std::string& clientID,
                  int pullThreadNum,
                  uint64_t tcpConnectTimeout,
                  uint64_t tcpTransportTryLockTimeout,
                  std::string unitName);
  virtual ~MQClientFactory();

  void start();
  void shutdown();
  bool registerProducer(MQProducer* pProducer);
  void unregisterProducer(MQProducer* pProducer);
  bool registerConsumer(MQConsumer* pConsumer);
  void unregisterConsumer(MQConsumer* pConsumer);

  void createTopic(const std::string& key,
                   const std::string& newTopic,
                   int queueNum,
                   const SessionCredentials& session_credentials);
  int64 minOffset(const MQMessageQueue& mq, const SessionCredentials& session_credentials);
  int64 maxOffset(const MQMessageQueue& mq, const SessionCredentials& session_credentials);
  int64 searchOffset(const MQMessageQueue& mq, int64 timestamp, const SessionCredentials& session_credentials);
  int64 earliestMsgStoreTime(const MQMessageQueue& mq, const SessionCredentials& session_credentials);
  MQMessageExt* viewMessage(const std::string& msgId, const SessionCredentials& session_credentials);
  QueryResult queryMessage(const std::string& topic,
                           const std::string& key,
                           int maxNum,
                           int64 begin,
                           int64 end,
                           const SessionCredentials& session_credentials);
  void endTransactionOneway(const MQMessageQueue& mq,
                            EndTransactionRequestHeader* requestHeader,
                            const SessionCredentials& sessionCredentials);
  void checkTransactionState(const std::string& addr,
                             const MQMessageExt& message,
                             const CheckTransactionStateRequestHeader& checkRequestHeader);
  MQClientAPIImpl* getMQClientAPIImpl() const;
  MQProducer* selectProducer(const std::string& group);
  MQConsumer* selectConsumer(const std::string& group);

  std::shared_ptr<TopicPublishInfo> topicRouteData2TopicPublishInfo(const std::string& topic, TopicRouteData* pRoute);

  void topicRouteData2TopicSubscribeInfo(const std::string& topic,
                                         TopicRouteData* pRoute,
                                         std::vector<MQMessageQueue>& mqs);

  FindBrokerResult* findBrokerAddressInSubscribe(const std::string& brokerName, int brokerId, bool onlyThisBroker);

  FindBrokerResult* findBrokerAddressInAdmin(const std::string& brokerName);

  std::string findBrokerAddressInPublish(const std::string& brokerName);

  std::shared_ptr<TopicPublishInfo> tryToFindTopicPublishInfo(const std::string& topic,
                                                                const SessionCredentials& session_credentials);

  void fetchSubscribeMessageQueues(const std::string& topic,
                                   std::vector<MQMessageQueue>& mqs,
                                   const SessionCredentials& session_credentials);

  bool updateTopicRouteInfoFromNameServer(const std::string& topic,
                                          const SessionCredentials& session_credentials,
                                          bool isDefault = false);
  void rebalanceImmediately();
  void doRebalanceByConsumerGroup(const std::string& consumerGroup);
  void sendHeartbeatToAllBroker();

  void findConsumerIds(const std::string& topic,
                       const std::string& group,
                       std::vector<string>& cids,
                       const SessionCredentials& session_credentials);
  void resetOffset(const std::string& group, const std::string& topic, const map<MQMessageQueue, int64>& offsetTable);
  ConsumerRunningInfo* consumerRunningInfo(const std::string& consumerGroup);
  bool getSessionCredentialFromConsumer(const std::string& consumerGroup, SessionCredentials& sessionCredentials);
  void addBrokerToAddrMap(const std::string& brokerName, map<int, string>& brokerAddrs);
  map<string, map<int, string>> getBrokerAddrMap();
  void clearBrokerAddrMap();

 private:
  void unregisterClient(const std::string& producerGroup,
                        const std::string& consumerGroup,
                        const SessionCredentials& session_credentials);
  TopicRouteData* getTopicRouteData(const std::string& topic);
  void addTopicRouteData(const std::string& topic, TopicRouteData* pTopicRouteData);
  HeartbeatData* prepareHeartbeatData();

  void startScheduledTask();

  // timer async callback
  void fetchNameServerAddr(boost::system::error_code& ec, boost::asio::deadline_timer* t);
  void updateTopicRouteInfo(boost::system::error_code& ec, boost::asio::deadline_timer* t);
  void timerCB_sendHeartbeatToAllBroker(boost::system::error_code& ec, boost::asio::deadline_timer* t);

  // consumer related operation
  void consumer_timerOperation();
  void persistAllConsumerOffset(boost::system::error_code& ec, boost::asio::deadline_timer* t);
  void doRebalance();
  void timerCB_doRebalance(boost::system::error_code& ec, boost::asio::deadline_timer* t);
  bool getSessionCredentialFromConsumerTable(SessionCredentials& sessionCredentials);
  bool addConsumerToTable(const std::string& consumerName, MQConsumer* pMQConsumer);
  void eraseConsumerFromTable(const std::string& consumerName);
  int getConsumerTableSize();
  void getTopicListFromConsumerSubscription(std::set<std::string>& topicList);
  void updateConsumerSubscribeTopicInfo(const std::string& topic, std::vector<MQMessageQueue> mqs);
  void insertConsumerInfoToHeartBeatData(HeartbeatData* pHeartbeatData);

  // producer related operation
  bool getSessionCredentialFromProducerTable(SessionCredentials& sessionCredentials);
  bool addProducerToTable(const std::string& producerName, MQProducer* pMQProducer);
  void eraseProducerFromTable(const std::string& producerName);
  int getProducerTableSize();
  void insertProducerInfoToHeartBeatData(HeartbeatData* pHeartbeatData);

  // topicPublishInfo related operation
  void addTopicInfoToTable(const std::string& topic, std::shared_ptr<TopicPublishInfo> pTopicPublishInfo);
  void eraseTopicInfoFromTable(const std::string& topic);
  bool isTopicInfoValidInTable(const std::string& topic);
  std::shared_ptr<TopicPublishInfo> getTopicPublishInfoFromTable(const std::string& topic);
  void getTopicListFromTopicPublishInfo(std::set<std::string>& topicList);

  void getSessionCredentialsFromOneOfProducerOrConsumer(SessionCredentials& session_credentials);

 private:
  std::string m_clientId;
  ServiceState m_serviceState;

  // group -> MQProducer
  typedef std::map<std::string, MQProducer*> MQPMAP;
  std::mutex m_producerTableMutex;
  MQPMAP m_producerTable;

  // group -> MQConsumer
  typedef std::map<std::string, MQConsumer*> MQCMAP;
  std::mutex m_consumerTableMutex;
  MQCMAP m_consumerTable;

  // Topic -> TopicRouteData
  typedef std::map<std::string, TopicRouteData*> TRDMAP;
  std::mutex m_topicRouteTableMutex;
  TRDMAP m_topicRouteTable;

  //<!-----brokerName
  //<!     ------brokerid;
  //<!     ------add;
  std::mutex m_brokerAddrlock;
  typedef std::map<string, std::map<int, std::string>> BrokerAddrMAP;
  BrokerAddrMAP m_brokerAddrTable;

  // topic -> TopicPublishInfo
  typedef std::map<std::string, std::shared_ptr<TopicPublishInfo>> TPMap;
  std::mutex m_topicPublishInfoTableMutex;
  TPMap m_topicPublishInfoTable;
  std::mutex m_factoryLock;
  std::mutex m_topicPublishInfoLock;

  // clientapi;
  std::unique_ptr<MQClientAPIImpl> m_pClientAPIImpl;
  std::unique_ptr<ClientRemotingProcessor> m_pClientRemotingProcessor;

  boost::asio::io_service m_async_ioService;
  std::unique_ptr<boost::thread> m_async_service_thread;

  boost::asio::io_service m_consumer_async_ioService;
  std::unique_ptr<boost::thread> m_consumer_async_service_thread;
};

}  // namespace rocketmq

#endif
