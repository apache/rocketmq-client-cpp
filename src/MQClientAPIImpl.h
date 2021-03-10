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
#ifndef ROCKETMQ_MQCLIENTAPIIMPL_H_
#define ROCKETMQ_MQCLIENTAPIIMPL_H_

#include "CommunicationMode.h"
#include "DefaultMQProducerImpl.h"
#include "KVTable.h"
#include "MQClientInstance.h"
#include "MQException.h"
#include "MQMessageExt.h"
#include "PullCallback.h"
#include "SendCallback.h"
#include "SendResult.h"
#include "TopicConfig.h"
#include "TopicList.h"
#include "TopicPublishInfo.hpp"
#include "protocol/body/LockBatchRequestBody.hpp"
#include "protocol/body/TopicRouteData.hpp"
#include "protocol/body/UnlockBatchRequestBody.hpp"
#include "protocol/header/CommandHeader.h"
#include "protocol/heartbeat/HeartbeatData.hpp"

namespace rocketmq {

class TcpRemotingClient;
class ClientRemotingProcessor;
class RPCHook;
class InvokeCallback;

/**
 * wrap all RPC API
 */
class MQClientAPIImpl {
 public:
  MQClientAPIImpl(ClientRemotingProcessor* clientRemotingProcessor,
                  RPCHookPtr rpcHook,
                  const MQClientConfig& clientConfig);
  virtual ~MQClientAPIImpl();

  void start();
  void shutdown();

  void updateNameServerAddressList(const std::string& addrs);

  void createTopic(const std::string& addr, const std::string& defaultTopic, TopicConfig topicConfig);

  SendResult* sendMessage(const std::string& addr,
                          const std::string& brokerName,
                          const MessagePtr msg,
                          std::unique_ptr<SendMessageRequestHeader> requestHeader,
                          int timeoutMillis,
                          CommunicationMode communicationMode,
                          DefaultMQProducerImplPtr producer);
  SendResult* sendMessage(const std::string& addr,
                          const std::string& brokerName,
                          const MessagePtr msg,
                          std::unique_ptr<SendMessageRequestHeader> requestHeader,
                          int timeoutMillis,
                          CommunicationMode communicationMode,
                          SendCallback* sendCallback,
                          TopicPublishInfoPtr topicPublishInfo,
                          MQClientInstancePtr instance,
                          int retryTimesWhenSendFailed,
                          DefaultMQProducerImplPtr producer);
  SendResult* processSendResponse(const std::string& brokerName, const MessagePtr msg, RemotingCommand* pResponse);

  PullResult* pullMessage(const std::string& addr,
                          PullMessageRequestHeader* requestHeader,
                          int timeoutMillis,
                          CommunicationMode communicationMode,
                          PullCallback* pullCallback);
  PullResult* processPullResponse(RemotingCommand* pResponse);

  MQMessageExt viewMessage(const std::string& addr, int64_t phyoffset, int timeoutMillis);

  int64_t searchOffset(const std::string& addr,
                       const std::string& topic,
                       int queueId,
                       int64_t timestamp,
                       int timeoutMillis);

  int64_t getMaxOffset(const std::string& addr, const std::string& topic, int queueId, int timeoutMillis);
  int64_t getMinOffset(const std::string& addr, const std::string& topic, int queueId, int timeoutMillis);

  int64_t getEarliestMsgStoretime(const std::string& addr, const std::string& topic, int queueId, int timeoutMillis);

  void getConsumerIdListByGroup(const std::string& addr,
                                const std::string& consumerGroup,
                                std::vector<std::string>& cids,
                                int timeoutMillis);

  int64_t queryConsumerOffset(const std::string& addr,
                              QueryConsumerOffsetRequestHeader* requestHeader,
                              int timeoutMillis);

  void updateConsumerOffset(const std::string& addr,
                            UpdateConsumerOffsetRequestHeader* requestHeader,
                            int timeoutMillis);
  void updateConsumerOffsetOneway(const std::string& addr,
                                  UpdateConsumerOffsetRequestHeader* requestHeader,
                                  int timeoutMillis);

  void sendHearbeat(const std::string& addr, HeartbeatData* heartbeatData, long timeoutMillis);
  void unregisterClient(const std::string& addr,
                        const std::string& clientID,
                        const std::string& producerGroup,
                        const std::string& consumerGroup);

  void endTransactionOneway(const std::string& addr,
                            EndTransactionRequestHeader* requestHeader,
                            const std::string& remark);

  void consumerSendMessageBack(const std::string& addr,
                               MessageExtPtr msg,
                               const std::string& consumerGroup,
                               int delayLevel,
                               int timeoutMillis,
                               int maxConsumeRetryTimes);

  void lockBatchMQ(const std::string& addr,
                   LockBatchRequestBody* requestBody,
                   std::vector<MQMessageQueue>& mqs,
                   int timeoutMillis);
  void unlockBatchMQ(const std::string& addr,
                     UnlockBatchRequestBody* requestBody,
                     int timeoutMillis,
                     bool oneway = false);

  TopicRouteData* getTopicRouteInfoFromNameServer(const std::string& topic, int timeoutMillis);

  TopicList* getTopicListFromNameServer();

  int wipeWritePermOfBroker(const std::string& namesrvAddr, const std::string& brokerName, int timeoutMillis);

  void deleteTopicInBroker(const std::string& addr, const std::string& topic, int timeoutMillis);
  void deleteTopicInNameServer(const std::string& addr, const std::string& topic, int timeoutMillis);

  void deleteSubscriptionGroup(const std::string& addr, const std::string& groupName, int timeoutMillis);

  std::string getKVConfigByValue(const std::string& projectNamespace,
                                 const std::string& projectGroup,
                                 int timeoutMillis);
  void deleteKVConfigByValue(const std::string& projectNamespace, const std::string& projectGroup, int timeoutMillis);

  KVTable getKVListByNamespace(const std::string& projectNamespace, int timeoutMillis);

 public:
  TcpRemotingClient* getRemotingClient() { return remoting_client_.get(); }

 private:
  friend class SendCallbackWrap;

  SendResult* sendMessageSync(const std::string& addr,
                              const std::string& brokerName,
                              const MessagePtr msg,
                              RemotingCommand& request,
                              int timeoutMillis);

  void sendMessageAsync(const std::string& addr,
                        const std::string& brokerName,
                        const MessagePtr msg,
                        RemotingCommand&& request,
                        SendCallback* sendCallback,
                        TopicPublishInfoPtr topicPublishInfo,
                        MQClientInstancePtr instance,
                        int64_t timeoutMilliseconds,
                        int retryTimesWhenSendFailed,
                        DefaultMQProducerImplPtr producer);

  void sendMessageAsyncImpl(std::unique_ptr<InvokeCallback>& cbw, int64_t timeoutMillis);

  PullResult* pullMessageSync(const std::string& addr, RemotingCommand& request, int timeoutMillis);

  void pullMessageAsync(const std::string& addr,
                        RemotingCommand& request,
                        int timeoutMillis,
                        PullCallback* pullCallback);

 private:
  std::unique_ptr<TcpRemotingClient> remoting_client_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_MQCLIENTAPIIMPL_H_
