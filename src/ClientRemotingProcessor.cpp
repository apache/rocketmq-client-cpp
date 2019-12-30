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

#include "CommandHeader.h"
#include "ConsumerRunningInfo.h"
#include "MQDecoder.h"
#include "MQProtos.h"

namespace rocketmq {

ClientRemotingProcessor::ClientRemotingProcessor(MQClientInstance* mqClientFactory)
    : m_mqClientFactory(mqClientFactory) {}

ClientRemotingProcessor::~ClientRemotingProcessor() = default;

RemotingCommand* ClientRemotingProcessor::processRequest(const std::string& addr, RemotingCommand* request) {
  LOG_DEBUG("request Command received:processRequest, addr:%s, code:%d", addr.data(), request->getCode());
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
  return nullptr;
}

RemotingCommand* ClientRemotingProcessor::resetOffset(RemotingCommand* request) {
  auto* responseHeader = request->decodeCommandCustomHeader<ResetOffsetRequestHeader>();
  auto requestBody = request->getBody();
  if (requestBody != nullptr && requestBody->getSize() > 0) {
    std::unique_ptr<ResetOffsetBody> body(ResetOffsetBody::Decode(*requestBody));
    if (body != nullptr) {
      m_mqClientFactory->resetOffset(responseHeader->getGroup(), responseHeader->getTopic(), body->getOffsetTable());
    } else {
      LOG_ERROR("resetOffset failed as received data could not be unserialized");
    }
  }
  return nullptr;  // as resetOffset is oneWayRPC, do not need return any response
}

ResetOffsetBody* ResetOffsetBody::Decode(MemoryBlock& mem) {
  Json::Value root = RemotingSerializable::fromJson(mem);
  Json::Value qds = root["offsetTable"];
  std::unique_ptr<ResetOffsetBody> body(new ResetOffsetBody());
  for (unsigned int i = 0; i < qds.size(); i++) {
    Json::Value qd = qds[i];
    MQMessageQueue mq(qd["brokerName"].asString(), qd["topic"].asString(), qd["queueId"].asInt());
    int64_t offset = qd["offset"].asInt64();
    body->setOffsetTable(mq, offset);
  }
  return body.release();
}

std::map<MQMessageQueue, int64_t> ResetOffsetBody::getOffsetTable() {
  return m_offsetTable;
}

void ResetOffsetBody::setOffsetTable(MQMessageQueue mq, int64_t offset) {
  m_offsetTable[mq] = offset;
}

RemotingCommand* ClientRemotingProcessor::getConsumerRunningInfo(const std::string& addr, RemotingCommand* request) {
  auto* requestHeader = request->decodeCommandCustomHeader<GetConsumerRunningInfoRequestHeader>();
  LOG_INFO("getConsumerRunningInfo:%s", requestHeader->getConsumerGroup().c_str());

  RemotingCommand* response =
      new RemotingCommand(request->getCode(), "CPP", request->getVersion(), request->getOpaque(), request->getFlag(),
                          request->getRemark(), nullptr);

  std::unique_ptr<ConsumerRunningInfo> runningInfo(
      m_mqClientFactory->consumerRunningInfo(requestHeader->getConsumerGroup()));
  if (runningInfo != nullptr) {
    if (requestHeader->isJstackEnable()) {
      /*string jstack = UtilAll::jstack();
       consumerRunningInfo->setJstack(jstack);*/
    }
    response->setCode(SUCCESS_VALUE);
    std::string body = runningInfo->encode();
    response->setBody(body);
  } else {
    response->setCode(SYSTEM_ERROR);
    response->setRemark("The Consumer Group not exist in this consumer");
  }

  return response;
}

RemotingCommand* ClientRemotingProcessor::notifyConsumerIdsChanged(RemotingCommand* request) {
  auto* requestHeader = request->decodeCommandCustomHeader<NotifyConsumerIdsChangedRequestHeader>();
  LOG_INFO("notifyConsumerIdsChanged:%s", requestHeader->getConsumerGroup().c_str());
  m_mqClientFactory->rebalanceImmediately();
  return nullptr;
}

RemotingCommand* ClientRemotingProcessor::checkTransactionState(const std::string& addr, RemotingCommand* request) {
  auto* requestHeader = request->decodeCommandCustomHeader<CheckTransactionStateRequestHeader>();
  assert(requestHeader != nullptr);

  auto requestBody = request->getBody();
  if (requestBody != nullptr && requestBody->getSize() > 0) {
    MQMessageExtPtr2 messageExt = MQDecoder::decode(*requestBody);
    if (messageExt != nullptr) {
      const auto& transactionId = messageExt->getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
      if (!transactionId.empty()) {
        messageExt->setTransactionId(transactionId);
      }
      const auto& group = messageExt->getProperty(MQMessageConst::PROPERTY_PRODUCER_GROUP);
      if (!group.empty()) {
        auto* producer = m_mqClientFactory->selectProducer(group);
        if (producer != nullptr) {
          producer->checkTransactionState(addr, messageExt, requestHeader);
        } else {
          LOG_DEBUG_NEW("checkTransactionState, pick producer by group[{}] failed", group);
        }
      } else {
        LOG_WARN_NEW("checkTransactionState, pick producer group failed");
      }
    } else {
      LOG_WARN_NEW("checkTransactionState, decode message failed");
    }
  } else {
    LOG_ERROR_NEW("checkTransactionState, request body is empty, request header: {}", requestHeader->toString());
  }

  return nullptr;
}

}  // namespace rocketmq
