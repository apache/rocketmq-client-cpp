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
#ifndef ROCKETMQ_MQCLIENTINSTANCE_H_
#define ROCKETMQ_MQCLIENTINSTANCE_H_

#include <memory>
#include <mutex>
#include <set>
#include <utility>

#include "protocol/body/ConsumerRunningInfo.h"
#include "FindBrokerResult.hpp"
#include "MQClientConfig.h"
#include "MQException.h"
#include "MQConsumerInner.h"
#include "MQMessageQueue.h"
#include "MQProducerInner.h"
#include "ServiceState.h"
#include "TopicPublishInfo.hpp"
#include "protocol/body/TopicRouteData.hpp"
#include "protocol/heartbeat/HeartbeatData.hpp"
#include "concurrent/executor.hpp"

namespace rocketmq {

class RPCHook;
typedef std::shared_ptr<RPCHook> RPCHookPtr;

class MQClientAPIImpl;
class MQAdminImpl;
class ClientRemotingProcessor;
class RebalanceService;
class PullMessageService;

class MQClientInstance;
typedef std::shared_ptr<MQClientInstance> MQClientInstancePtr;

class MQClientInstance {
 public:
  MQClientInstance(const MQClientConfig& clientConfig, const std::string& clientId);
  MQClientInstance(const MQClientConfig& clientConfig, const std::string& clientId, RPCHookPtr rpcHook);
  virtual ~MQClientInstance();

  static TopicPublishInfoPtr topicRouteData2TopicPublishInfo(const std::string& topic, TopicRouteDataPtr route);
  static std::vector<MQMessageQueue> topicRouteData2TopicSubscribeInfo(const std::string& topic,
                                                                       TopicRouteDataPtr route);

  const std::string& getClientId() const;
  std::string getNamesrvAddr() const;

  void start();
  void shutdown();
  bool isRunning();

  bool registerProducer(const std::string& group, MQProducerInner* producer);
  void unregisterProducer(const std::string& group);

  bool registerConsumer(const std::string& group, MQConsumerInner* consumer);
  void unregisterConsumer(const std::string& group);

  void updateTopicRouteInfoFromNameServer();
  bool updateTopicRouteInfoFromNameServer(const std::string& topic, bool isDefault = false);

  void sendHeartbeatToAllBrokerWithLock();

  void rebalanceImmediately();
  void doRebalance();

  MQProducerInner* selectProducer(const std::string& group);
  MQConsumerInner* selectConsumer(const std::string& group);

  FindBrokerResult* findBrokerAddressInAdmin(const std::string& brokerName);
  std::string findBrokerAddressInPublish(const std::string& brokerName);
  FindBrokerResult* findBrokerAddressInSubscribe(const std::string& brokerName, int brokerId, bool onlyThisBroker);

  void findConsumerIds(const std::string& topic, const std::string& group, std::vector<std::string>& cids);

  std::string findBrokerAddrByTopic(const std::string& topic);

  void resetOffset(const std::string& group,
                   const std::string& topic,
                   const std::map<MQMessageQueue, int64_t>& offsetTable);

  ConsumerRunningInfo* consumerRunningInfo(const std::string& consumerGroup);

 public:
  TopicPublishInfoPtr tryToFindTopicPublishInfo(const std::string& topic);

  TopicRouteDataPtr getTopicRouteData(const std::string& topic);

 public:
  MQClientAPIImpl* getMQClientAPIImpl() const { return mq_client_api_impl_.get(); }
  MQAdminImpl* getMQAdminImpl() const { return mq_admin_impl_.get(); }
  PullMessageService* getPullMessageService() const { return pull_message_service_.get(); }

 private:
  typedef std::map<std::string, std::map<int, std::string>> BrokerAddrMAP;

  void unregisterClientWithLock(const std::string& producerGroup, const std::string& consumerGroup);
  void unregisterClient(const std::string& producerGroup, const std::string& consumerGroup);

  void addBrokerToAddrTable(const std::string& brokerName, const std::map<int, std::string>& brokerAddrs);
  void resetBrokerAddrTable(BrokerAddrMAP&& table);
  void clearBrokerAddrTable();
  BrokerAddrMAP getBrokerAddrTable();

  void cleanOfflineBroker();
  bool isBrokerAddrExistInTopicRouteTable(const std::string& addr);

  // scheduled task
  void startScheduledTask();
  void updateTopicRouteInfoPeriodically();
  void sendHeartbeatToAllBrokerPeriodically();
  void persistAllConsumerOffsetPeriodically();

  // topic route
  bool topicRouteDataIsChange(TopicRouteData* old, TopicRouteData* now);
  void addTopicRouteData(const std::string& topic, TopicRouteDataPtr topicRouteData);

  // heartbeat
  void sendHeartbeatToAllBroker();
  HeartbeatData* prepareHeartbeatData();
  void insertConsumerInfoToHeartBeatData(HeartbeatData* pHeartbeatData);
  void insertProducerInfoToHeartBeatData(HeartbeatData* pHeartbeatData);

  // offset
  void persistAllConsumerOffset();

  // rebalance
  void doRebalanceByConsumerGroup(const std::string& consumerGroup);

  // consumer related operation
  bool addConsumerToTable(const std::string& consumerName, MQConsumerInner* consumer);
  void eraseConsumerFromTable(const std::string& consumerName);
  int getConsumerTableSize();
  void getTopicListFromConsumerSubscription(std::set<std::string>& topicList);
  void updateConsumerTopicSubscribeInfo(const std::string& topic, std::vector<MQMessageQueue> subscribeInfo);

  // producer related operation
  bool addProducerToTable(const std::string& producerName, MQProducerInner* producer);
  void eraseProducerFromTable(const std::string& producerName);
  int getProducerTableSize();
  void getTopicListFromTopicPublishInfo(std::set<std::string>& topicList);
  void updateProducerTopicPublishInfo(const std::string& topic, TopicPublishInfoPtr publishInfo);

  // topicPublishInfo related operation
  void addTopicInfoToTable(const std::string& topic, TopicPublishInfoPtr pTopicPublishInfo);
  void eraseTopicInfoFromTable(const std::string& topic);
  TopicPublishInfoPtr getTopicPublishInfoFromTable(const std::string& topic);
  bool isTopicInfoValidInTable(const std::string& topic);

 private:
  std::string client_id_;
  volatile ServiceState service_state_;

  // group -> MQProducer
  typedef std::map<std::string, MQProducerInner*> MQPMAP;
  MQPMAP producer_table_;
  std::mutex producer_table_mutex_;

  // group -> MQConsumer
  typedef std::map<std::string, MQConsumerInner*> MQCMAP;
  MQCMAP consumer_table_;
  std::mutex consumer_table_mutex_;

  // Topic -> TopicRouteData
  typedef std::map<std::string, TopicRouteDataPtr> TRDMAP;
  TRDMAP topic_route_table_;
  std::mutex topic_route_table_mutex_;

  // brokerName -> [ brokerid : addr ]
  BrokerAddrMAP broker_addr_table_;
  std::mutex broker_addr_table_mutex_;

  // topic -> TopicPublishInfo
  typedef std::map<std::string, TopicPublishInfoPtr> TPMAP;
  TPMAP topic_publish_info_table_;
  std::mutex topic_publish_info_table_mutex_;

  // topic -> <broker, time>
  typedef std::map<std::string, std::pair<std::string, uint64_t>> TBAMAP;
  TBAMAP topic_broker_addr_table_;
  std::mutex topic_broker_addr_table_mutex_;

  std::timed_mutex lock_namesrv_;
  std::timed_mutex lock_heartbeat_;

  std::unique_ptr<MQClientAPIImpl> mq_client_api_impl_;
  std::unique_ptr<MQAdminImpl> mq_admin_impl_;
  std::unique_ptr<ClientRemotingProcessor> client_remoting_processor_;

  std::unique_ptr<RebalanceService> rebalance_service_;
  std::unique_ptr<PullMessageService> pull_message_service_;
  scheduled_thread_pool_executor scheduled_executor_service_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQCLIENTINSTANCE_H_
