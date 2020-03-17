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
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/thread.hpp>
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
//<!************************************************************************
class TopicPublishInfo;
class MQClientFactory {
 public:
  MQClientFactory(const string& clientID,
                  int pullThreadNum,
                  uint64_t tcpConnectTimeout,
                  uint64_t tcpTransportTryLockTimeout,
                  string unitName,
                  bool enableSsl,
                  const std::string& sslPropertyFile);
  MQClientFactory(const string& clientID, bool enableSsl, const std::string& sslPropertyFile);
  virtual ~MQClientFactory();

  virtual void start();
  virtual void shutdown();
  virtual bool registerProducer(MQProducer* pProducer);
  virtual void unregisterProducer(MQProducer* pProducer);
  virtual bool registerConsumer(MQConsumer* pConsumer);
  virtual void unregisterConsumer(MQConsumer* pConsumer);

  void createTopic(const string& key,
                   const string& newTopic,
                   int queueNum,
                   const SessionCredentials& session_credentials);
  int64 minOffset(const MQMessageQueue& mq, const SessionCredentials& session_credentials);
  int64 maxOffset(const MQMessageQueue& mq, const SessionCredentials& session_credentials);
  int64 searchOffset(const MQMessageQueue& mq, int64 timestamp, const SessionCredentials& session_credentials);
  int64 earliestMsgStoreTime(const MQMessageQueue& mq, const SessionCredentials& session_credentials);
  MQMessageExt* viewMessage(const string& msgId, const SessionCredentials& session_credentials);
  QueryResult queryMessage(const string& topic,
                           const string& key,
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
  virtual MQClientAPIImpl* getMQClientAPIImpl();
  MQProducer* selectProducer(const string& group);
  MQConsumer* selectConsumer(const string& group);

  boost::shared_ptr<TopicPublishInfo> topicRouteData2TopicPublishInfo(const string& topic, TopicRouteData* pRoute);

  void topicRouteData2TopicSubscribeInfo(const string& topic, TopicRouteData* pRoute, vector<MQMessageQueue>& mqs);

  FindBrokerResult* findBrokerAddressInSubscribe(const string& brokerName, int brokerId, bool onlyThisBroker);

  FindBrokerResult* findBrokerAddressInAdmin(const string& brokerName);

  virtual string findBrokerAddressInPublish(const string& brokerName);

  virtual boost::shared_ptr<TopicPublishInfo> tryToFindTopicPublishInfo(const string& topic,
                                                                        const SessionCredentials& session_credentials);

  void fetchSubscribeMessageQueues(const string& topic,
                                   vector<MQMessageQueue>& mqs,
                                   const SessionCredentials& session_credentials);

  bool updateTopicRouteInfoFromNameServer(const string& topic,
                                          const SessionCredentials& session_credentials,
                                          bool isDefault = false);
  void rebalanceImmediately();
  void doRebalanceByConsumerGroup(const string& consumerGroup);
  virtual void sendHeartbeatToAllBroker();

  void cleanOfflineBrokers();

  void findConsumerIds(const string& topic,
                       const string& group,
                       vector<string>& cids,
                       const SessionCredentials& session_credentials);
  void resetOffset(const string& group, const string& topic, const map<MQMessageQueue, int64>& offsetTable);
  ConsumerRunningInfo* consumerRunningInfo(const string& consumerGroup);
  bool getSessionCredentialFromConsumer(const string& consumerGroup, SessionCredentials& sessionCredentials);
  void addBrokerToAddrMap(const string& brokerName, map<int, string>& brokerAddrs);
  map<string, map<int, string>> getBrokerAddrMap();
  void clearBrokerAddrMap();

  bool isBrokerAddressInUse(const std::string& address);

 private:
  void unregisterClient(const string& producerGroup,
                        const string& consumerGroup,
                        const SessionCredentials& session_credentials);
  TopicRouteData* getTopicRouteData(const string& topic);
  void addTopicRouteData(const string& topic, TopicRouteData* pTopicRouteData);
  HeartbeatData* prepareHeartbeatData();

  void startScheduledTask(bool startFetchNSService = true);
  //<!timer async callback
  void fetchNameServerAddr(boost::system::error_code& ec, boost::shared_ptr<boost::asio::deadline_timer> t);
  void updateTopicRouteInfo(boost::system::error_code& ec, boost::shared_ptr<boost::asio::deadline_timer> t);
  void timerCB_sendHeartbeatToAllBroker(boost::system::error_code& ec,
                                        boost::shared_ptr<boost::asio::deadline_timer> t);

  void timerCB_cleanOfflineBrokers(boost::system::error_code& ec, boost::shared_ptr<boost::asio::deadline_timer> t);

  // consumer related operation
  void consumer_timerOperation();
  void persistAllConsumerOffset(boost::system::error_code& ec, boost::shared_ptr<boost::asio::deadline_timer> t);
  void doRebalance();
  void timerCB_doRebalance(boost::system::error_code& ec, boost::shared_ptr<boost::asio::deadline_timer> t);
  bool getSessionCredentialFromConsumerTable(SessionCredentials& sessionCredentials);
  void eraseConsumerFromTable(const string& consumerName);
  int getConsumerTableSize();
  void getTopicListFromConsumerSubscription(set<string>& topicList);
  void updateConsumerSubscribeTopicInfo(const string& topic, vector<MQMessageQueue> mqs);
  void insertConsumerInfoToHeartBeatData(HeartbeatData* pHeartbeatData);

  // producer related operation
  bool getSessionCredentialFromProducerTable(SessionCredentials& sessionCredentials);
  void eraseProducerFromTable(const string& producerName);
  int getProducerTableSize();
  void insertProducerInfoToHeartBeatData(HeartbeatData* pHeartbeatData);

  // topicPublishInfo related operation
  void addTopicInfoToTable(const string& topic, boost::shared_ptr<TopicPublishInfo> pTopicPublishInfo);
  void eraseTopicInfoFromTable(const string& topic);
  bool isTopicInfoValidInTable(const string& topic);
  boost::shared_ptr<TopicPublishInfo> getTopicPublishInfoFromTable(const string& topic);
  void getTopicListFromTopicPublishInfo(set<string>& topicList);

  void getSessionCredentialsFromOneOfProducerOrConsumer(SessionCredentials& session_credentials);

 protected:
  string m_clientId;
  unique_ptr<MQClientAPIImpl> m_pClientAPIImpl;
  unique_ptr<ClientRemotingProcessor> m_pClientRemotingProcessor;

  bool addProducerToTable(const string& producerName, MQProducer* pMQProducer);
  bool addConsumerToTable(const string& consumerName, MQConsumer* pMQConsumer);

 private:
  string m_nameSrvDomain;  // per clientId
  ServiceState m_serviceState;
  bool m_bFetchNSService;

  //<! group --> MQProducer;
  typedef map<string, MQProducer*> MQPMAP;
  boost::mutex m_producerTableMutex;
  MQPMAP m_producerTable;

  //<! group --> MQConsumer;
  typedef map<string, MQConsumer*> MQCMAP;
  // Changed to recursive mutex due to avoid deadlock issue:
  boost::recursive_mutex m_consumerTableMutex;
  MQCMAP m_consumerTable;

  //<! Topic---> TopicRouteData
  typedef map<string, TopicRouteData*> TRDMAP;
  boost::mutex m_topicRouteTableMutex;
  TRDMAP m_topicRouteTable;

  //<!-----brokerName
  //<!     ------brokerid;
  //<!     ------add;
  boost::mutex m_brokerAddrlock;
  typedef map<string, map<int, string>> BrokerAddrMAP;
  BrokerAddrMAP m_brokerAddrTable;

  //<!topic ---->TopicPublishInfo> ;
  typedef map<string, boost::shared_ptr<TopicPublishInfo>> TPMap;
  boost::mutex m_topicPublishInfoTableMutex;
  TPMap m_topicPublishInfoTable;
  boost::mutex m_factoryLock;
  boost::mutex m_topicPublishInfoLock;

  boost::asio::io_service m_async_ioService;
  unique_ptr<boost::thread> m_async_service_thread;

  boost::asio::io_service m_consumer_async_ioService;
  unique_ptr<boost::thread> m_consumer_async_service_thread;
};

}  // namespace rocketmq

#endif
