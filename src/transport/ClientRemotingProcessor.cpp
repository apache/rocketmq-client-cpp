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
#include "ClientRemotingProcessor.h"
#include "ClientRPCHook.h"
#include "ConsumerRunningInfo.h"
#include "MQClientFactory.h"
#include "UtilAll.h"

namespace rocketmq {

ClientRemotingProcessor::ClientRemotingProcessor(MQClientFactory* mqClientFactory)
    : m_mqClientFactory(mqClientFactory) {}

ClientRemotingProcessor::~ClientRemotingProcessor() {}

RemotingCommand* ClientRemotingProcessor::processRequest(const string& addr, RemotingCommand* request) {
  LOG_INFO("request Command received:processRequest, addr:%s, code:%d", addr.data(), request->getCode());
  switch (request->getCode()) {
    case CHECK_TRANSACTION_STATE:
      return checkTransactionState(addr, request);
      break;
    case NOTIFY_CONSUMER_IDS_CHANGED:
      return notifyConsumerIdsChanged(request);
      break;
    case RESET_CONSUMER_CLIENT_OFFSET:  // oneWayRPC
      return resetOffset(request);
    case GET_CONSUMER_STATUS_FROM_CLIENT:
      // return getConsumeStatus( request);
      break;
    case GET_CONSUMER_RUNNING_INFO:
      return getConsumerRunningInfo(addr, request);
      break;
    case CONSUME_MESSAGE_DIRECTLY:
      // return consumeMessageDirectly( request);
      break;
    default:
      break;
  }
  return NULL;
}

RemotingCommand* ClientRemotingProcessor::resetOffset(RemotingCommand* request) {
  request->SetExtHeader(request->getCode());
  const MemoryBlock* pbody = request->GetBody();
  if (pbody->getSize()) {
    ResetOffsetBody* offsetBody = ResetOffsetBody::Decode(pbody);
    ResetOffsetRequestHeader* offsetHeader = (ResetOffsetRequestHeader*)request->getCommandHeader();
    if (offsetBody) {
      m_mqClientFactory->resetOffset(offsetHeader->getGroup(), offsetHeader->getTopic(), offsetBody->getOffsetTable());
      delete offsetBody;
      offsetBody = nullptr;
    } else {
      LOG_ERROR("resetOffset failed as received data could not be unserialized");
    }
  }
  return NULL;  // as resetOffset is oneWayRPC, do not need return any response
}

std::map<MQMessageQueue, int64> ResetOffsetBody::getOffsetTable() {
  return m_offsetTable;
}

void ResetOffsetBody::setOffsetTable(const MQMessageQueue& mq, int64 offset) {
  m_offsetTable[mq] = offset;
}

ResetOffsetBody* ResetOffsetBody::Decode(const MemoryBlock* mem) {
  const char* const pData = static_cast<const char*>(mem->getData());
  Json::Reader reader;
  Json::Value root;
  const char* begin = pData;
  const char* end = pData + mem->getSize();

  if (!reader.parse(begin, end, root, true)) {
    LOG_ERROR("ResetOffsetBody::Decode fail");
    return NULL;
  }

  ResetOffsetBody* rfb = new ResetOffsetBody();
  Json::Value qds = root["offsetTable"];
  for (unsigned int i = 0; i < qds.size(); i++) {
    MQMessageQueue mq;
    Json::Value qd = qds[i];
    mq.setBrokerName(qd["brokerName"].asString());
    mq.setQueueId(qd["queueId"].asInt());
    mq.setTopic(qd["topic"].asString());
    int64 offset = qd["offset"].asInt64();
    LOG_INFO("ResetOffsetBody brokerName:%s, queueID:%d, topic:%s, offset:%lld", mq.getBrokerName().c_str(),
             mq.getQueueId(), mq.getTopic().c_str(), offset);
    rfb->setOffsetTable(mq, offset);
  }
  return rfb;
}

RemotingCommand* ClientRemotingProcessor::getConsumerRunningInfo(const string& addr, RemotingCommand* request) {
  request->SetExtHeader(request->getCode());
  GetConsumerRunningInfoRequestHeader* requestHeader =
      (GetConsumerRunningInfoRequestHeader*)request->getCommandHeader();
  LOG_INFO("getConsumerRunningInfo:%s", requestHeader->getConsumerGroup().c_str());

  RemotingCommand* pResponse =
      new RemotingCommand(request->getCode(), "CPP", request->getVersion(), request->getOpaque(), request->getFlag(),
                          request->getRemark(), NULL);

  unique_ptr<ConsumerRunningInfo> runningInfo(
      m_mqClientFactory->consumerRunningInfo(requestHeader->getConsumerGroup()));
  if (runningInfo) {
    if (requestHeader->isJstackEnable()) {
      /*string jstack = UtilAll::jstack();
       consumerRunningInfo->setJstack(jstack);*/
    }
    pResponse->setCode(SUCCESS_VALUE);
    string body = runningInfo->encode();
    pResponse->SetBody(body.c_str(), body.length());
    pResponse->setMsgBody(body);
  } else {
    pResponse->setCode(SYSTEM_ERROR);
    pResponse->setRemark("The Consumer Group not exist in this consumer");
  }

  SessionCredentials sessionCredentials;
  m_mqClientFactory->getSessionCredentialFromConsumer(requestHeader->getConsumerGroup(), sessionCredentials);
  ClientRPCHook rpcHook(sessionCredentials);
  rpcHook.doBeforeRequest(addr, *pResponse);
  pResponse->Encode();
  return pResponse;
}

RemotingCommand* ClientRemotingProcessor::notifyConsumerIdsChanged(RemotingCommand* request) {
  request->SetExtHeader(request->getCode());
  NotifyConsumerIdsChangedRequestHeader* requestHeader =
      (NotifyConsumerIdsChangedRequestHeader*)request->getCommandHeader();
  if (requestHeader == nullptr) {
    LOG_ERROR("notifyConsumerIdsChanged requestHeader null");
    return NULL;
  }
  string group = requestHeader->getGroup();
  LOG_INFO("notifyConsumerIdsChanged:%s", group.c_str());
  m_mqClientFactory->doRebalanceByConsumerGroup(requestHeader->getGroup());
  return NULL;
}

RemotingCommand* ClientRemotingProcessor::checkTransactionState(const std::string& addr, RemotingCommand* request) {
  if (!request) {
    LOG_ERROR("checkTransactionState request null");
    return nullptr;
  }

  LOG_INFO("checkTransactionState addr:%s, request: %s", addr.data(), request->ToString().data());

  request->SetExtHeader(request->getCode());
  CheckTransactionStateRequestHeader* requestHeader = (CheckTransactionStateRequestHeader*)request->getCommandHeader();
  if (!requestHeader) {
    LOG_ERROR("checkTransactionState CheckTransactionStateRequestHeader requestHeader null");
    return nullptr;
  }
  LOG_INFO("checkTransactionState request: %s", requestHeader->toString().data());

  const MemoryBlock* block = request->GetBody();
  if (block && block->getSize() > 0) {
    std::vector<MQMessageExt> mqvec;
    MQDecoder::decodes(block, mqvec);
    if (mqvec.size() == 0) {
      LOG_ERROR("checkTransactionState decodes MQMessageExt fail, request:%s", requestHeader->toString().data());
      return nullptr;
    }

    MQMessageExt& messageExt = mqvec[0];
    string transactionId = messageExt.getProperty(MQMessage::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
    if (transactionId != "") {
      messageExt.setTransactionId(transactionId);
    }

    m_mqClientFactory->checkTransactionState(addr, messageExt, *requestHeader);
  } else {
    LOG_ERROR("checkTransactionState getbody null or size 0, request Header:%s", requestHeader->toString().data());
  }
  return nullptr;
}

}  // namespace rocketmq
