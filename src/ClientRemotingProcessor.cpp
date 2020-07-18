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

#include "ConsumerRunningInfo.h"
#include "MessageDecoder.h"
#include "MQProtos.h"
#include "MessageAccessor.h"
#include "MessageSysFlag.h"
#include "RequestFutureTable.h"
#include "SocketUtil.h"
#include "protocol/body/ResetOffsetBody.h"
#include "protocol/header/CommandHeader.h"
#include "protocol/header/ReplyMessageRequestHeader.h"

namespace rocketmq {

ClientRemotingProcessor::ClientRemotingProcessor(MQClientInstance* clientInstance) : m_clientInstance(clientInstance) {}

ClientRemotingProcessor::~ClientRemotingProcessor() = default;

RemotingCommand* ClientRemotingProcessor::processRequest(TcpTransportPtr channel, RemotingCommand* request) {
  const auto& addr = channel->getPeerAddrAndPort();
  LOG_DEBUG_NEW("processRequest, code:{}, addr:{}", request->getCode(), addr);
  switch (request->getCode()) {
    case CHECK_TRANSACTION_STATE:
      return checkTransactionState(addr, request);
    case NOTIFY_CONSUMER_IDS_CHANGED:
      return notifyConsumerIdsChanged(request);
    case RESET_CONSUMER_CLIENT_OFFSET:  // oneWayRPC
      return resetOffset(request);
    case GET_CONSUMER_STATUS_FROM_CLIENT:
      // return getConsumeStatus( request);
      break;
    case GET_CONSUMER_RUNNING_INFO:
      return getConsumerRunningInfo(addr, request);
    case CONSUME_MESSAGE_DIRECTLY:
      // return consumeMessageDirectly( request);
      break;
    case PUSH_REPLY_MESSAGE_TO_CLIENT:
      return receiveReplyMessage(request);
    default:
      break;
  }
  return nullptr;
}

RemotingCommand* ClientRemotingProcessor::checkTransactionState(const std::string& addr, RemotingCommand* request) {
  auto* requestHeader = request->decodeCommandCustomHeader<CheckTransactionStateRequestHeader>();
  assert(requestHeader != nullptr);

  auto requestBody = request->getBody();
  if (requestBody != nullptr && requestBody->getSize() > 0) {
    MessageExtPtr messageExt = MessageDecoder::decode(*requestBody);
    if (messageExt != nullptr) {
      const auto& transactionId = messageExt->getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
      if (!transactionId.empty()) {
        messageExt->setTransactionId(transactionId);
      }
      const auto& group = messageExt->getProperty(MQMessageConst::PROPERTY_PRODUCER_GROUP);
      if (!group.empty()) {
        auto* producer = m_clientInstance->selectProducer(group);
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

RemotingCommand* ClientRemotingProcessor::notifyConsumerIdsChanged(RemotingCommand* request) {
  auto* requestHeader = request->decodeCommandCustomHeader<NotifyConsumerIdsChangedRequestHeader>();
  LOG_INFO_NEW("notifyConsumerIdsChanged, group:{}", requestHeader->getConsumerGroup());
  m_clientInstance->rebalanceImmediately();
  return nullptr;
}

RemotingCommand* ClientRemotingProcessor::resetOffset(RemotingCommand* request) {
  auto* responseHeader = request->decodeCommandCustomHeader<ResetOffsetRequestHeader>();
  auto requestBody = request->getBody();
  if (requestBody != nullptr && requestBody->getSize() > 0) {
    std::unique_ptr<ResetOffsetBody> body(ResetOffsetBody::Decode(*requestBody));
    if (body != nullptr) {
      m_clientInstance->resetOffset(responseHeader->getGroup(), responseHeader->getTopic(), body->getOffsetTable());
    } else {
      LOG_ERROR("resetOffset failed as received data could not be unserialized");
    }
  }
  return nullptr;  // as resetOffset is oneWayRPC, do not need return any response
}

RemotingCommand* ClientRemotingProcessor::getConsumerRunningInfo(const std::string& addr, RemotingCommand* request) {
  auto* requestHeader = request->decodeCommandCustomHeader<GetConsumerRunningInfoRequestHeader>();
  LOG_INFO("getConsumerRunningInfo, group:{}", requestHeader->getConsumerGroup());

  std::unique_ptr<RemotingCommand> response(
      new RemotingCommand(MQResponseCode::SYSTEM_ERROR, "not set any response code"));

  std::unique_ptr<ConsumerRunningInfo> runningInfo(
      m_clientInstance->consumerRunningInfo(requestHeader->getConsumerGroup()));
  if (runningInfo != nullptr) {
    if (requestHeader->isJstackEnable()) {
      /*string jstack = UtilAll::jstack();
       consumerRunningInfo->setJstack(jstack);*/
    }
    response->setCode(SUCCESS);
    auto body = runningInfo->encode();
    response->setBody(body);
  } else {
    response->setCode(SYSTEM_ERROR);
    response->setRemark("The Consumer Group not exist in this consumer");
  }

  return response.release();
}

RemotingCommand* ClientRemotingProcessor::receiveReplyMessage(RemotingCommand* request) {
  std::unique_ptr<RemotingCommand> response(
      new RemotingCommand(MQResponseCode::SYSTEM_ERROR, "not set any response code"));

  auto receiveTime = UtilAll::currentTimeMillis();
  auto* requestHeader = request->decodeCommandCustomHeader<ReplyMessageRequestHeader>();

  try {
    std::unique_ptr<MQMessageExt> msg(new MQMessageExt);

    msg->setTopic(requestHeader->getTopic());
    msg->setQueueId(requestHeader->getQueueId());
    msg->setStoreTimestamp(requestHeader->getStoreTimestamp());

    if (!requestHeader->getBornHost().empty()) {
      msg->setBornHost(string2SocketAddress(requestHeader->getBornHost()));
    }

    if (!requestHeader->getStoreHost().empty()) {
      msg->setStoreHost(string2SocketAddress(requestHeader->getStoreHost()));
    }

    auto body = request->getBody();
    if ((requestHeader->getSysFlag() & MessageSysFlag::CompressedFlag) == MessageSysFlag::CompressedFlag) {
      std::string outbody;
      if (UtilAll::inflate(body->getData(), body->getSize(), outbody)) {
        msg->setBody(std::move(outbody));
      } else {
        LOG_WARN_NEW("err when uncompress constant");
      }
    } else {
      msg->setBody(body->getData(), body->getSize());
    }

    msg->setFlag(requestHeader->getFlag());
    MessageAccessor::setProperties(*msg, MessageDecoder::string2messageProperties(requestHeader->getProperties()));
    MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_REPLY_MESSAGE_ARRIVE_TIME,
                                 UtilAll::to_string(receiveTime));
    msg->setBornTimestamp(requestHeader->getBornTimestamp());
    msg->setReconsumeTimes(requestHeader->getReconsumeTimes());
    LOG_DEBUG_NEW("receive reply message:{}", msg->toString());

    processReplyMessage(std::move(msg));

    response->setCode(MQResponseCode::SUCCESS);
    response->setRemark(null);
  } catch (const std::exception& e) {
    LOG_WARN_NEW("unknown err when receiveReplyMsg, {}", e.what());
    response->setCode(MQResponseCode::SYSTEM_ERROR);
    response->setRemark("process reply message fail");
  }

  return response.release();
}

void ClientRemotingProcessor::processReplyMessage(std::unique_ptr<MQMessageExt> replyMsg) {
  const auto& correlationId = replyMsg->getProperty(MQMessageConst::PROPERTY_CORRELATION_ID);
  auto requestResponseFuture = RequestFutureTable::removeRequestFuture(correlationId);
  if (requestResponseFuture != nullptr) {
    requestResponseFuture->putResponseMessage(std::move(replyMsg));
    requestResponseFuture->executeRequestCallback();
  } else {
    auto bornHost = replyMsg->getBornHostString();
    LOG_WARN_NEW("receive reply message, but not matched any request, CorrelationId: {} , reply from host: {}",
                 correlationId, bornHost);
  }
}

}  // namespace rocketmq
