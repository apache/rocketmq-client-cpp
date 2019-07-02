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

#include "MQClientAPIImpl.h"

#include "CommunicationMode.h"
#include "Logging.h"
#include "MQDecoder.h"
#include "PullResultExt.h"

namespace rocketmq {

MQClientAPIImpl::MQClientAPIImpl(const string& mqClientId,
                                 ClientRemotingProcessor* clientRemotingProcessor,
                                 int pullThreadNum,
                                 uint64_t tcpConnectTimeout,
                                 uint64_t tcpTransportTryLockTimeout,
                                 string unitName)
    : m_mqClientId(mqClientId) {
  m_pRemotingClient.reset(new TcpRemotingClient(pullThreadNum, tcpConnectTimeout, tcpTransportTryLockTimeout));
  m_pRemotingClient->registerProcessor(CHECK_TRANSACTION_STATE, clientRemotingProcessor);
  m_pRemotingClient->registerProcessor(RESET_CONSUMER_CLIENT_OFFSET, clientRemotingProcessor);
  m_pRemotingClient->registerProcessor(GET_CONSUMER_STATUS_FROM_CLIENT, clientRemotingProcessor);
  m_pRemotingClient->registerProcessor(GET_CONSUMER_RUNNING_INFO, clientRemotingProcessor);
  m_pRemotingClient->registerProcessor(NOTIFY_CONSUMER_IDS_CHANGED, clientRemotingProcessor);
  m_pRemotingClient->registerProcessor(CONSUME_MESSAGE_DIRECTLY, clientRemotingProcessor);
}

MQClientAPIImpl::~MQClientAPIImpl() {
  m_pRemotingClient = NULL;
}

void MQClientAPIImpl::stopAllTcpTransportThread() {
  m_pRemotingClient->stopAllTcpTransportThread();
}

void MQClientAPIImpl::updateNameServerAddr(const string& addrs) {
  if (m_pRemotingClient != NULL)
    m_pRemotingClient->updateNameServerAddressList(addrs);
}

void MQClientAPIImpl::callSignatureBeforeRequest(const string& addr,
                                                 RemotingCommand& request,
                                                 const SessionCredentials& session_credentials) {
  ClientRPCHook rpcHook(session_credentials);
  rpcHook.doBeforeRequest(addr, request);
}

// Note: all request rules: throw exception if got broker error response,
// exclude getTopicRouteInfoFromNameServer and unregisterClient
void MQClientAPIImpl::createTopic(const string& addr,
                                  const string& defaultTopic,
                                  TopicConfig topicConfig,
                                  const SessionCredentials& sessionCredentials) {
  string topicWithProjectGroup = topicConfig.getTopicName();
  CreateTopicRequestHeader* requestHeader = new CreateTopicRequestHeader();
  requestHeader->topic = (topicWithProjectGroup);
  requestHeader->defaultTopic = (defaultTopic);
  requestHeader->readQueueNums = (topicConfig.getReadQueueNums());
  requestHeader->writeQueueNums = (topicConfig.getWriteQueueNums());
  requestHeader->perm = (topicConfig.getPerm());
  requestHeader->topicFilterType = (topicConfig.getTopicFilterType());

  RemotingCommand request(UPDATE_AND_CREATE_TOPIC, requestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> response(m_pRemotingClient->invokeSync(addr, request));

  if (response) {
    switch (response->getCode()) {
      case SUCCESS_VALUE:
        return;
      default:
        break;
    }
    THROW_MQEXCEPTION(MQBrokerException, response->getRemark(), response->getCode());
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
}

void MQClientAPIImpl::endTransactionOneway(std::string addr,
                                           EndTransactionRequestHeader* requestHeader,
                                           std::string remark,
                                           const SessionCredentials& sessionCredentials) {
  RemotingCommand request(END_TRANSACTION, requestHeader);
  request.setRemark(remark);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();
  m_pRemotingClient->invokeOneway(addr, request);
  return;
}

SendResult MQClientAPIImpl::sendMessage(const string& addr,
                                        const string& brokerName,
                                        const MQMessage& msg,
                                        SendMessageRequestHeader* pRequestHeader,
                                        int timeoutMillis,
                                        int maxRetrySendTimes,
                                        int communicationMode,
                                        SendCallback* pSendCallback,
                                        const SessionCredentials& sessionCredentials) {
  RemotingCommand request(SEND_MESSAGE, pRequestHeader);
  string body = msg.getBody();
  request.SetBody(body.c_str(), body.length());
  request.setMsgBody(body);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  switch (communicationMode) {
    case ComMode_ONEWAY:
      m_pRemotingClient->invokeOneway(addr, request);
      break;
    case ComMode_ASYNC:
      sendMessageAsync(addr, brokerName, msg, request, pSendCallback, timeoutMillis, maxRetrySendTimes, 1);
      break;
    case ComMode_SYNC:
      return sendMessageSync(addr, brokerName, msg, request, timeoutMillis);
    default:
      break;
  }
  return SendResult();
}

void MQClientAPIImpl::sendHearbeat(const string& addr,
                                   HeartbeatData* pHeartbeatData,
                                   const SessionCredentials& sessionCredentials) {
  RemotingCommand request(HEART_BEAT, NULL);

  string body;
  pHeartbeatData->Encode(body);
  request.SetBody(body.data(), body.length());
  request.setMsgBody(body);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  if (m_pRemotingClient->invokeHeartBeat(addr, request)) {
    LOG_INFO("sendheartbeat to broker:%s success", addr.c_str());
  }
}

void MQClientAPIImpl::unregisterClient(const string& addr,
                                       const string& clientID,
                                       const string& producerGroup,
                                       const string& consumerGroup,
                                       const SessionCredentials& sessionCredentials) {
  LOG_INFO("unregisterClient to broker:%s", addr.c_str());
  RemotingCommand request(UNREGISTER_CLIENT, new UnregisterClientRequestHeader(clientID, producerGroup, consumerGroup));
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> response(m_pRemotingClient->invokeSync(addr, request));

  if (response) {
    switch (response->getCode()) {
      case SUCCESS_VALUE:
        LOG_INFO("unregisterClient to:%s success", addr.c_str());
        return;
      default:
        break;
    }
    LOG_WARN("unregisterClient fail:%s,%d", response->getRemark().c_str(), response->getCode());
  }
}

// return NULL if got no response or error response
TopicRouteData* MQClientAPIImpl::getTopicRouteInfoFromNameServer(const string& topic,
                                                                 int timeoutMillis,
                                                                 const SessionCredentials& sessionCredentials) {
  RemotingCommand request(GET_ROUTEINTO_BY_TOPIC, new GetRouteInfoRequestHeader(topic));
  callSignatureBeforeRequest("", request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> pResponse(m_pRemotingClient->invokeSync("", request, timeoutMillis));

  if (pResponse != NULL) {
    if (((*(pResponse->GetBody())).getSize() == 0) || ((*(pResponse->GetBody())).getData() != NULL)) {
      switch (pResponse->getCode()) {
        case SUCCESS_VALUE: {
          const MemoryBlock* pbody = pResponse->GetBody();
          if (pbody->getSize()) {
            TopicRouteData* topicRoute = TopicRouteData::Decode(pbody);
            return topicRoute;
          }
        }
        case TOPIC_NOT_EXIST: {
          return NULL;
        }
        default:
          break;
      }
      LOG_WARN("%s,%d", pResponse->getRemark().c_str(), pResponse->getCode());
      return NULL;
    }
  }
  return NULL;
}

TopicList* MQClientAPIImpl::getTopicListFromNameServer(const SessionCredentials& sessionCredentials) {
  RemotingCommand request(GET_ALL_TOPIC_LIST_FROM_NAMESERVER, NULL);
  callSignatureBeforeRequest("", request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> pResponse(m_pRemotingClient->invokeSync("", request));
  if (pResponse != NULL) {
    if (((*(pResponse->GetBody())).getSize() == 0) || ((*(pResponse->GetBody())).getData() != NULL)) {
      switch (pResponse->getCode()) {
        case SUCCESS_VALUE: {
          const MemoryBlock* pbody = pResponse->GetBody();
          if (pbody->getSize()) {
            TopicList* topicList = TopicList::Decode(pbody);
            return topicList;
          }
        }
        default:
          break;
      }

      THROW_MQEXCEPTION(MQClientException, pResponse->getRemark(), pResponse->getCode());
    }
  }
  return NULL;
}

int MQClientAPIImpl::wipeWritePermOfBroker(const string& namesrvAddr, const string& brokerName, int timeoutMillis) {
  return 0;
}

void MQClientAPIImpl::deleteTopicInBroker(const string& addr, const string& topic, int timeoutMillis) {}

void MQClientAPIImpl::deleteTopicInNameServer(const string& addr, const string& topic, int timeoutMillis) {}

void MQClientAPIImpl::deleteSubscriptionGroup(const string& addr, const string& groupName, int timeoutMillis) {}

string MQClientAPIImpl::getKVConfigByValue(const string& projectNamespace,
                                           const string& projectGroup,
                                           int timeoutMillis) {
  return "";
}

KVTable MQClientAPIImpl::getKVListByNamespace(const string& projectNamespace, int timeoutMillis) {
  return KVTable();
}

void MQClientAPIImpl::deleteKVConfigByValue(const string& projectNamespace,
                                            const string& projectGroup,
                                            int timeoutMillis) {}

SendResult MQClientAPIImpl::sendMessageSync(const string& addr,
                                            const string& brokerName,
                                            const MQMessage& msg,
                                            RemotingCommand& request,
                                            int timeoutMillis) {
  //<!block util response;
  std::unique_ptr<RemotingCommand> pResponse(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));
  if (pResponse != NULL) {
    try {
      SendResult result = processSendResponse(brokerName, msg, pResponse.get());
      LOG_DEBUG("sendMessageSync success:%s to addr:%s,brokername:%s, send status:%d", msg.toString().c_str(),
                addr.c_str(), brokerName.c_str(), (int)result.getSendStatus());
      return result;
    } catch (...) {
      LOG_ERROR("send error");
    }
  }
  THROW_MQEXCEPTION(MQClientException, "response is null", -1);
}

void MQClientAPIImpl::sendMessageAsync(const std::string& addr,
                                       const std::string& brokerName,
                                       const MQMessage& msg,
                                       RemotingCommand& request,
                                       SendCallback* pSendCallback,
                                       int64 timeoutMilliseconds,
                                       int maxRetryTimes,
                                       int retrySendTimes) {
  int64 begin_time = UtilAll::currentTimeMillis();
  // delete in future;
  auto* cbw = new SendCallbackWrap(brokerName, msg, pSendCallback, this);

  LOG_DEBUG("sendMessageAsync request:%s, timeout:%lld, maxRetryTimes:%d retrySendTimes:%d", request.ToString().data(),
            timeoutMilliseconds, maxRetryTimes, retrySendTimes);

  if (m_pRemotingClient->invokeAsync(addr, request, cbw, timeoutMilliseconds, maxRetryTimes, retrySendTimes) == false) {
    LOG_WARN("invokeAsync failed to addr:%s,topic:%s, timeout:%lld, maxRetryTimes:%d, retrySendTimes:%d", addr.c_str(),
             msg.getTopic().data(), timeoutMilliseconds, maxRetryTimes, retrySendTimes);
    // when getTcp return false, need consider retrySendTimes
    int retry_time = retrySendTimes + 1;
    int64 time_out = timeoutMilliseconds - (UtilAll::currentTimeMillis() - begin_time);
    while (retry_time < maxRetryTimes && time_out > 0) {
      begin_time = UtilAll::currentTimeMillis();
      if (m_pRemotingClient->invokeAsync(addr, request, cbw, time_out, maxRetryTimes, retry_time) == false) {
        retry_time += 1;
        time_out = time_out - (UtilAll::currentTimeMillis() - begin_time);
        LOG_WARN("invokeAsync retry failed to addr:%s,topic:%s, timeout:%lld, maxRetryTimes:%d, retrySendTimes:%d",
                 addr.c_str(), msg.getTopic().data(), time_out, maxRetryTimes, retry_time);
        continue;
      } else {
        return;  // invokeAsync success
      }
    }

    LOG_ERROR("sendMessageAsync failed to addr:%s,topic:%s, timeout:%lld, maxRetryTimes:%d, retrySendTimes:%d",
              addr.c_str(), msg.getTopic().data(), time_out, maxRetryTimes, retrySendTimes);

    if (cbw) {
      cbw->onException();
      deleteAndZero(cbw);
    } else {
      THROW_MQEXCEPTION(MQClientException, "sendMessageAsync failed", -1);
    }
  }
}

PullResult* MQClientAPIImpl::pullMessage(const string& addr,
                                         PullMessageRequestHeader* pRequestHeader,
                                         int timeoutMillis,
                                         int communicationMode,
                                         PullCallback* pullCallback,
                                         void* pArg,
                                         const SessionCredentials& sessionCredentials) {
  RemotingCommand request(PULL_MESSAGE, pRequestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  switch (communicationMode) {
    case ComMode_ONEWAY:
      break;
    case ComMode_ASYNC:
      pullMessageAsync(addr, request, timeoutMillis, pullCallback, pArg);
      break;
    case ComMode_SYNC:
      return pullMessageSync(addr, request, timeoutMillis);
    default:
      break;
  }

  return NULL;
}

void MQClientAPIImpl::pullMessageAsync(const string& addr,
                                       RemotingCommand& request,
                                       int timeoutMillis,
                                       PullCallback* pullCallback,
                                       void* pArg) {
  // delete in future
  auto* cbw = new PullCallbackWarp(pullCallback, this, pArg);
  if (!m_pRemotingClient->invokeAsync(addr, request, cbw, timeoutMillis)) {
    LOG_ERROR("pullMessageAsync failed of addr:%s, mq:%s", addr.c_str(),
              static_cast<AsyncArg*>(pArg)->mq.toString().data());
    deleteAndZero(cbw);
    THROW_MQEXCEPTION(MQClientException, "pullMessageAsync failed", -1);
  }
}

PullResult* MQClientAPIImpl::pullMessageSync(const string& addr, RemotingCommand& request, int timeoutMillis) {
  std::unique_ptr<RemotingCommand> pResponse(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));
  if (pResponse != NULL) {
    if (((*(pResponse->GetBody())).getSize() == 0) || ((*(pResponse->GetBody())).getData() != NULL)) {
      try {
        PullResult* pullResult = processPullResponse(pResponse.get());  // pullMessage will handle
                                                                        // exception from
                                                                        // processPullResponse
        return pullResult;
      } catch (MQException& e) {
        LOG_ERROR(e.what());
        return NULL;
      }
    }
  }
  return NULL;
}

SendResult MQClientAPIImpl::processSendResponse(const string& brokerName,
                                                const MQMessage& msg,
                                                RemotingCommand* pResponse) {
  SendStatus sendStatus = SEND_OK;
  int res = 0;
  switch (pResponse->getCode()) {
    case FLUSH_DISK_TIMEOUT:
      sendStatus = SEND_FLUSH_DISK_TIMEOUT;
      break;
    case FLUSH_SLAVE_TIMEOUT:
      sendStatus = SEND_FLUSH_SLAVE_TIMEOUT;
      break;
    case SLAVE_NOT_AVAILABLE:
      sendStatus = SEND_SLAVE_NOT_AVAILABLE;
      break;
    case SUCCESS_VALUE:
      sendStatus = SEND_OK;
      break;
    default:
      res = -1;
      break;
  }
  if (res == 0) {
    SendMessageResponseHeader* responseHeader = (SendMessageResponseHeader*)pResponse->getCommandHeader();
    MQMessageQueue messageQueue(msg.getTopic(), brokerName, responseHeader->queueId);
    string unique_msgId = msg.getProperty(MQMessage::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
    return SendResult(sendStatus, unique_msgId, responseHeader->msgId, messageQueue, responseHeader->queueOffset);
  }
  LOG_ERROR("processSendResponse error remark:%s, error code:%d", (pResponse->getRemark()).c_str(),
            pResponse->getCode());
  THROW_MQEXCEPTION(MQClientException, pResponse->getRemark(), pResponse->getCode());
}

PullResult* MQClientAPIImpl::processPullResponse(RemotingCommand* pResponse) {
  PullStatus pullStatus = NO_NEW_MSG;
  switch (pResponse->getCode()) {
    case SUCCESS_VALUE:
      pullStatus = FOUND;
      break;
    case PULL_NOT_FOUND:
      pullStatus = NO_NEW_MSG;
      break;
    case PULL_RETRY_IMMEDIATELY:
      pullStatus = NO_MATCHED_MSG;
      break;
    case PULL_OFFSET_MOVED:
      pullStatus = OFFSET_ILLEGAL;
      break;
    default:
      THROW_MQEXCEPTION(MQBrokerException, pResponse->getRemark(), pResponse->getCode());
      break;
  }

  PullMessageResponseHeader* responseHeader = static_cast<PullMessageResponseHeader*>(pResponse->getCommandHeader());

  if (!responseHeader) {
    LOG_ERROR("processPullResponse:responseHeader is NULL");
    THROW_MQEXCEPTION(MQClientException, "processPullResponse:responseHeader is NULL", -1);
  }
  //<!get body,delete outsite;
  MemoryBlock bodyFromResponse = *(pResponse->GetBody());  // response data judgement had been done outside
                                                           // of processPullResponse
  if (bodyFromResponse.getSize() == 0) {
    if (pullStatus != FOUND) {
      return new PullResultExt(pullStatus, responseHeader->nextBeginOffset, responseHeader->minOffset,
                               responseHeader->maxOffset, (int)responseHeader->suggestWhichBrokerId);
    } else {
      THROW_MQEXCEPTION(MQClientException, "memoryBody size is 0, but pullStatus equals found", -1);
    }
  } else {
    return new PullResultExt(pullStatus, responseHeader->nextBeginOffset, responseHeader->minOffset,
                             responseHeader->maxOffset, (int)responseHeader->suggestWhichBrokerId, bodyFromResponse);
  }
}

//<!***************************************************************************
int64 MQClientAPIImpl::getMinOffset(const string& addr,
                                    const string& topic,
                                    int queueId,
                                    int timeoutMillis,
                                    const SessionCredentials& sessionCredentials) {
  GetMinOffsetRequestHeader* pRequestHeader = new GetMinOffsetRequestHeader();
  pRequestHeader->topic = topic;
  pRequestHeader->queueId = queueId;

  RemotingCommand request(GET_MIN_OFFSET, pRequestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> response(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));

  if (response) {
    switch (response->getCode()) {
      case SUCCESS_VALUE: {
        GetMinOffsetResponseHeader* responseHeader = (GetMinOffsetResponseHeader*)response->getCommandHeader();

        int64 offset = responseHeader->offset;
        return offset;
      }
      default:
        break;
    }
    THROW_MQEXCEPTION(MQBrokerException, response->getRemark(), response->getCode());
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
}

int64 MQClientAPIImpl::getMaxOffset(const string& addr,
                                    const string& topic,
                                    int queueId,
                                    int timeoutMillis,
                                    const SessionCredentials& sessionCredentials) {
  GetMaxOffsetRequestHeader* pRequestHeader = new GetMaxOffsetRequestHeader();
  pRequestHeader->topic = topic;
  pRequestHeader->queueId = queueId;

  RemotingCommand request(GET_MAX_OFFSET, pRequestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> response(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));

  if (response) {
    switch (response->getCode()) {
      case SUCCESS_VALUE: {
        GetMaxOffsetResponseHeader* responseHeader = (GetMaxOffsetResponseHeader*)response->getCommandHeader();

        int64 offset = responseHeader->offset;
        return offset;
      }
      default:
        break;
    }
    THROW_MQEXCEPTION(MQBrokerException, response->getRemark(), response->getCode());
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
}

int64 MQClientAPIImpl::searchOffset(const string& addr,
                                    const string& topic,
                                    int queueId,
                                    uint64_t timestamp,
                                    int timeoutMillis,
                                    const SessionCredentials& sessionCredentials) {
  SearchOffsetRequestHeader* pRequestHeader = new SearchOffsetRequestHeader();
  pRequestHeader->topic = topic;
  pRequestHeader->queueId = queueId;
  pRequestHeader->timestamp = timestamp;

  RemotingCommand request(SEARCH_OFFSET_BY_TIMESTAMP, pRequestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> response(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));

  if (response) {
    switch (response->getCode()) {
      case SUCCESS_VALUE: {
        SearchOffsetResponseHeader* responseHeader = (SearchOffsetResponseHeader*)response->getCommandHeader();

        int64 offset = responseHeader->offset;
        return offset;
      }
      default:
        break;
    }
    THROW_MQEXCEPTION(MQBrokerException, response->getRemark(), response->getCode());
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
}

MQMessageExt* MQClientAPIImpl::viewMessage(const string& addr,
                                           int64 phyoffset,
                                           int timeoutMillis,
                                           const SessionCredentials& sessionCredentials) {
  ViewMessageRequestHeader* pRequestHeader = new ViewMessageRequestHeader();
  pRequestHeader->offset = phyoffset;

  RemotingCommand request(VIEW_MESSAGE_BY_ID, pRequestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> response(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));

  if (response) {
    switch (response->getCode()) {
      case SUCCESS_VALUE: {
      }
      default:
        break;
    }
    THROW_MQEXCEPTION(MQBrokerException, response->getRemark(), response->getCode());
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
}

int64 MQClientAPIImpl::getEarliestMsgStoretime(const string& addr,
                                               const string& topic,
                                               int queueId,
                                               int timeoutMillis,
                                               const SessionCredentials& sessionCredentials) {
  GetEarliestMsgStoretimeRequestHeader* pRequestHeader = new GetEarliestMsgStoretimeRequestHeader();
  pRequestHeader->topic = topic;
  pRequestHeader->queueId = queueId;

  RemotingCommand request(GET_EARLIEST_MSG_STORETIME, pRequestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> response(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));

  if (response) {
    switch (response->getCode()) {
      case SUCCESS_VALUE: {
        GetEarliestMsgStoretimeResponseHeader* responseHeader =
            (GetEarliestMsgStoretimeResponseHeader*)response->getCommandHeader();

        int64 timestamp = responseHeader->timestamp;
        return timestamp;
      }
      default:
        break;
    }
    THROW_MQEXCEPTION(MQBrokerException, response->getRemark(), response->getCode());
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
}

void MQClientAPIImpl::getConsumerIdListByGroup(const string& addr,
                                               const string& consumerGroup,
                                               std::vector<string>& cids,
                                               int timeoutMillis,
                                               const SessionCredentials& sessionCredentials) {
  GetConsumerListByGroupRequestHeader* pRequestHeader = new GetConsumerListByGroupRequestHeader();
  pRequestHeader->consumerGroup = consumerGroup;

  RemotingCommand request(GET_CONSUMER_LIST_BY_GROUP, pRequestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> pResponse(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));

  if (pResponse != NULL) {
    if ((pResponse->GetBody()->getSize() == 0) || (pResponse->GetBody()->getData() != NULL)) {
      switch (pResponse->getCode()) {
        case SUCCESS_VALUE: {
          const MemoryBlock* pbody = pResponse->GetBody();
          if (pbody->getSize()) {
            GetConsumerListByGroupResponseBody::Decode(pbody, cids);
            return;
          }
        }
        default:
          break;
      }
      THROW_MQEXCEPTION(MQBrokerException, pResponse->getRemark(), pResponse->getCode());
    }
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
}

int64 MQClientAPIImpl::queryConsumerOffset(const string& addr,
                                           QueryConsumerOffsetRequestHeader* pRequestHeader,
                                           int timeoutMillis,
                                           const SessionCredentials& sessionCredentials) {
  RemotingCommand request(QUERY_CONSUMER_OFFSET, pRequestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> response(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));

  if (response) {
    switch (response->getCode()) {
      case SUCCESS_VALUE: {
        QueryConsumerOffsetResponseHeader* responseHeader =
            (QueryConsumerOffsetResponseHeader*)response->getCommandHeader();
        int64 consumerOffset = responseHeader->offset;
        return consumerOffset;
      }
      default:
        break;
    }
    THROW_MQEXCEPTION(MQBrokerException, response->getRemark(), response->getCode());
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
  return -1;
}

void MQClientAPIImpl::updateConsumerOffset(const string& addr,
                                           UpdateConsumerOffsetRequestHeader* pRequestHeader,
                                           int timeoutMillis,
                                           const SessionCredentials& sessionCredentials) {
  RemotingCommand request(UPDATE_CONSUMER_OFFSET, pRequestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> response(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));

  if (response) {
    switch (response->getCode()) {
      case SUCCESS_VALUE: {
        return;
      }
      default:
        break;
    }
    THROW_MQEXCEPTION(MQBrokerException, response->getRemark(), response->getCode());
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
}

void MQClientAPIImpl::updateConsumerOffsetOneway(const string& addr,
                                                 UpdateConsumerOffsetRequestHeader* pRequestHeader,
                                                 int timeoutMillis,
                                                 const SessionCredentials& sessionCredentials) {
  RemotingCommand request(UPDATE_CONSUMER_OFFSET, pRequestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  m_pRemotingClient->invokeOneway(addr, request);
}

void MQClientAPIImpl::consumerSendMessageBack(MQMessageExt& msg,
                                              const string& consumerGroup,
                                              int delayLevel,
                                              int timeoutMillis,
                                              const SessionCredentials& sessionCredentials) {
  ConsumerSendMsgBackRequestHeader* pRequestHeader = new ConsumerSendMsgBackRequestHeader();
  pRequestHeader->group = consumerGroup;
  pRequestHeader->offset = msg.getCommitLogOffset();
  pRequestHeader->delayLevel = delayLevel;

  string addr = socketAddress2IPPort(msg.getStoreHost());
  RemotingCommand request(CONSUMER_SEND_MSG_BACK, pRequestHeader);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> response(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));

  if (response) {
    switch (response->getCode()) {
      case SUCCESS_VALUE: {
        return;
      }
      default:
        break;
    }
    THROW_MQEXCEPTION(MQBrokerException, response->getRemark(), response->getCode());
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
}

void MQClientAPIImpl::lockBatchMQ(const string& addr,
                                  LockBatchRequestBody* requestBody,
                                  std::vector<MQMessageQueue>& mqs,
                                  int timeoutMillis,
                                  const SessionCredentials& sessionCredentials) {
  RemotingCommand request(LOCK_BATCH_MQ, NULL);
  string body;
  requestBody->Encode(body);
  request.SetBody(body.data(), body.length());
  request.setMsgBody(body);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> pResponse(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));

  if (pResponse != NULL) {
    if (((*(pResponse->GetBody())).getSize() == 0) || ((*(pResponse->GetBody())).getData() != NULL)) {
      switch (pResponse->getCode()) {
        case SUCCESS_VALUE: {
          const MemoryBlock* pbody = pResponse->GetBody();
          if (pbody->getSize()) {
            LockBatchResponseBody::Decode(pbody, mqs);
          }
          return;
        } break;
        default:
          break;
      }
      THROW_MQEXCEPTION(MQBrokerException, pResponse->getRemark(), pResponse->getCode());
    }
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
}

void MQClientAPIImpl::unlockBatchMQ(const string& addr,
                                    UnlockBatchRequestBody* requestBody,
                                    int timeoutMillis,
                                    const SessionCredentials& sessionCredentials) {
  RemotingCommand request(UNLOCK_BATCH_MQ, NULL);
  string body;
  requestBody->Encode(body);
  request.SetBody(body.data(), body.length());
  request.setMsgBody(body);
  callSignatureBeforeRequest(addr, request, sessionCredentials);
  request.Encode();

  std::unique_ptr<RemotingCommand> pResponse(m_pRemotingClient->invokeSync(addr, request, timeoutMillis));

  if (pResponse != NULL) {
    switch (pResponse->getCode()) {
      case SUCCESS_VALUE: {
        return;
      } break;
      default:
        break;
    }
    THROW_MQEXCEPTION(MQBrokerException, pResponse->getRemark(), pResponse->getCode());
  }
  THROW_MQEXCEPTION(MQBrokerException, "response is null", -1);
}

}  // namespace rocketmq
