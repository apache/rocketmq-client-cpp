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

#include <cassert>
#include <cstring>

#include <typeindex>

#include "ClientRemotingProcessor.h"
#include "MQClientInstance.h"
#include "MessageBatch.h"
#include "MessageClientIDSetter.h"
#include "PullCallbackWrap.h"
#include "PullResultExt.hpp"
#include "SendCallbackWrap.h"
#include "TcpRemotingClient.h"
#include "protocol/body/LockBatchResponseBody.hpp"

namespace rocketmq {

MQClientAPIImpl::MQClientAPIImpl(ClientRemotingProcessor* clientRemotingProcessor,
                                 RPCHookPtr rpcHook,
                                 const MQClientConfig& clientConfig)
    : remoting_client_(new TcpRemotingClient(clientConfig.tcp_transport_worker_thread_nums(),
                                             clientConfig.tcp_transport_connect_timeout(),
                                             clientConfig.tcp_transport_try_lock_timeout())) {
  remoting_client_->registerRPCHook(rpcHook);
  remoting_client_->registerProcessor(CHECK_TRANSACTION_STATE, clientRemotingProcessor);
  remoting_client_->registerProcessor(NOTIFY_CONSUMER_IDS_CHANGED, clientRemotingProcessor);
  remoting_client_->registerProcessor(RESET_CONSUMER_CLIENT_OFFSET, clientRemotingProcessor);
  remoting_client_->registerProcessor(GET_CONSUMER_STATUS_FROM_CLIENT, clientRemotingProcessor);
  remoting_client_->registerProcessor(GET_CONSUMER_RUNNING_INFO, clientRemotingProcessor);
  remoting_client_->registerProcessor(CONSUME_MESSAGE_DIRECTLY, clientRemotingProcessor);
  remoting_client_->registerProcessor(PUSH_REPLY_MESSAGE_TO_CLIENT, clientRemotingProcessor);
}

MQClientAPIImpl::~MQClientAPIImpl() = default;

void MQClientAPIImpl::start() {
  remoting_client_->start();
}

void MQClientAPIImpl::shutdown() {
  remoting_client_->shutdown();
}

void MQClientAPIImpl::updateNameServerAddressList(const std::string& addrs) {
  // TODO: split addrs
  remoting_client_->updateNameServerAddressList(addrs);
}

void MQClientAPIImpl::createTopic(const std::string& addr, const std::string& defaultTopic, TopicConfig topicConfig) {
  auto* requestHeader = new CreateTopicRequestHeader();
  requestHeader->topic = topicConfig.topic_name();
  requestHeader->defaultTopic = defaultTopic;
  requestHeader->readQueueNums = topicConfig.read_queue_nums();
  requestHeader->writeQueueNums = topicConfig.write_queue_nums();
  requestHeader->perm = topicConfig.perm();
  requestHeader->topicFilterType = topicConfig.topic_filter_type();

  RemotingCommand request(UPDATE_AND_CREATE_TOPIC, requestHeader);

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      return;
    }
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

SendResult* MQClientAPIImpl::sendMessage(const std::string& addr,
                                         const std::string& brokerName,
                                         const MessagePtr msg,
                                         std::unique_ptr<SendMessageRequestHeader> requestHeader,
                                         int timeoutMillis,
                                         CommunicationMode communicationMode,
                                         DefaultMQProducerImplPtr producer) {
  return sendMessage(addr, brokerName, msg, std::move(requestHeader), timeoutMillis, communicationMode, nullptr,
                     nullptr, nullptr, 0, producer);
}

SendResult* MQClientAPIImpl::sendMessage(const std::string& addr,
                                         const std::string& brokerName,
                                         const MessagePtr msg,
                                         std::unique_ptr<SendMessageRequestHeader> requestHeader,
                                         int timeoutMillis,
                                         CommunicationMode communicationMode,
                                         SendCallback* sendCallback,
                                         TopicPublishInfoPtr topicPublishInfo,
                                         MQClientInstancePtr instance,
                                         int retryTimesWhenSendFailed,
                                         DefaultMQProducerImplPtr producer) {
  int code = SEND_MESSAGE;
  std::unique_ptr<CommandCustomHeader> header;

  const auto& msgType = msg->getProperty(MQMessageConst::PROPERTY_MESSAGE_TYPE);
  bool isReply = msgType == REPLY_MESSAGE_FLAG;
  if (isReply) {
    code = SEND_REPLY_MESSAGE_V2;
  } else if (msg->isBatch()) {
    code = SEND_BATCH_MESSAGE;
  } else {
    code = SEND_MESSAGE_V2;
  }

  if (code != SEND_MESSAGE && code != SEND_REPLY_MESSAGE) {
    header.reset(SendMessageRequestHeaderV2::createSendMessageRequestHeaderV2(requestHeader.get()));
  } else {
    header = std::move(requestHeader);
  }

  RemotingCommand request(code, header.release());
  request.set_body(msg->body());

  switch (communicationMode) {
    case CommunicationMode::ONEWAY:
      remoting_client_->invokeOneway(addr, request);
      return nullptr;
    case CommunicationMode::ASYNC:
      sendMessageAsync(addr, brokerName, msg, std::move(request), sendCallback, topicPublishInfo, instance,
                       timeoutMillis, retryTimesWhenSendFailed, producer);
      return nullptr;
    case CommunicationMode::SYNC:
      return sendMessageSync(addr, brokerName, msg, request, timeoutMillis);
    default:
      assert(false);
      break;
  }

  return nullptr;
}

void MQClientAPIImpl::sendMessageAsync(const std::string& addr,
                                       const std::string& brokerName,
                                       const MessagePtr msg,
                                       RemotingCommand&& request,
                                       SendCallback* sendCallback,
                                       TopicPublishInfoPtr topicPublishInfo,
                                       MQClientInstancePtr instance,
                                       int64_t timeoutMillis,
                                       int retryTimesWhenSendFailed,
                                       DefaultMQProducerImplPtr producer) {
  std::unique_ptr<InvokeCallback> cbw(
      new SendCallbackWrap(addr, brokerName, msg, std::forward<RemotingCommand>(request), sendCallback,
                           topicPublishInfo, instance, retryTimesWhenSendFailed, 0, producer));
  sendMessageAsyncImpl(cbw, timeoutMillis);
}

void MQClientAPIImpl::sendMessageAsyncImpl(std::unique_ptr<InvokeCallback>& cbw, int64_t timeoutMillis) {
  auto* scbw = static_cast<SendCallbackWrap*>(cbw.get());
  const auto& addr = scbw->getAddr();
  auto& request = scbw->getRemotingCommand();
  remoting_client_->invokeAsync(addr, request, cbw, timeoutMillis);
}

SendResult* MQClientAPIImpl::sendMessageSync(const std::string& addr,
                                             const std::string& brokerName,
                                             const MessagePtr msg,
                                             RemotingCommand& request,
                                             int timeoutMillis) {
  // block until response
  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  return processSendResponse(brokerName, msg, response.get());
}

SendResult* MQClientAPIImpl::processSendResponse(const std::string& brokerName,
                                                 const MessagePtr msg,
                                                 RemotingCommand* response) {
  SendStatus sendStatus = SEND_OK;
  switch (response->code()) {
    case FLUSH_DISK_TIMEOUT:
      sendStatus = SEND_FLUSH_DISK_TIMEOUT;
      break;
    case FLUSH_SLAVE_TIMEOUT:
      sendStatus = SEND_FLUSH_SLAVE_TIMEOUT;
      break;
    case SLAVE_NOT_AVAILABLE:
      sendStatus = SEND_SLAVE_NOT_AVAILABLE;
      break;
    case SUCCESS:
      sendStatus = SEND_OK;
      break;
    default:
      THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
      return nullptr;
  }

  auto* responseHeader = response->decodeCommandCustomHeader<SendMessageResponseHeader>();
  assert(responseHeader != nullptr);

  MQMessageQueue messageQueue(msg->topic(), brokerName, responseHeader->queueId);

  std::string uniqMsgId = MessageClientIDSetter::getUniqID(*msg);

  // MessageBatch
  if (msg->isBatch()) {
    const auto& messages = dynamic_cast<MessageBatch*>(msg.get())->messages();
    uniqMsgId.clear();
    uniqMsgId.reserve(33 * messages.size() + 1);
    for (const auto& message : messages) {
      uniqMsgId.append(MessageClientIDSetter::getUniqID(message));
      uniqMsgId.append(",");
    }
    if (!uniqMsgId.empty()) {
      uniqMsgId.resize(uniqMsgId.length() - 1);
    }
  }

  SendResult* sendResult =
      new SendResult(sendStatus, uniqMsgId, responseHeader->msgId, messageQueue, responseHeader->queueOffset);
  sendResult->set_transaction_id(responseHeader->transactionId);

  return sendResult;
}

PullResult* MQClientAPIImpl::pullMessage(const std::string& addr,
                                         PullMessageRequestHeader* requestHeader,
                                         int timeoutMillis,
                                         CommunicationMode communicationMode,
                                         PullCallback* pullCallback) {
  RemotingCommand request(PULL_MESSAGE, requestHeader);

  switch (communicationMode) {
    case CommunicationMode::ASYNC:
      pullMessageAsync(addr, request, timeoutMillis, pullCallback);
      return nullptr;
    case CommunicationMode::SYNC:
      return pullMessageSync(addr, request, timeoutMillis);
    default:
      assert(false);
      return nullptr;
  }
}

void MQClientAPIImpl::pullMessageAsync(const std::string& addr,
                                       RemotingCommand& request,
                                       int timeoutMillis,
                                       PullCallback* pullCallback) {
  std::unique_ptr<InvokeCallback> cbw(new PullCallbackWrap(pullCallback, this));
  remoting_client_->invokeAsync(addr, request, cbw, timeoutMillis);
}

PullResult* MQClientAPIImpl::pullMessageSync(const std::string& addr, RemotingCommand& request, int timeoutMillis) {
  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  return processPullResponse(response.get());
}

PullResult* MQClientAPIImpl::processPullResponse(RemotingCommand* response) {
  PullStatus pullStatus = NO_NEW_MSG;
  switch (response->code()) {
    case SUCCESS:
      pullStatus = FOUND;
      break;
    case PULL_NOT_FOUND:
      pullStatus = NO_NEW_MSG;
      break;
    case PULL_RETRY_IMMEDIATELY:
      if ("OFFSET_OVERFLOW_BADLY" == response->remark()) {
        pullStatus = NO_LATEST_MSG;
      } else {
        pullStatus = NO_MATCHED_MSG;
      }
      break;
    case PULL_OFFSET_MOVED:
      pullStatus = OFFSET_ILLEGAL;
      break;
    default:
      THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
  }

  // return of decodeCommandCustomHeader is non-null
  auto* responseHeader = response->decodeCommandCustomHeader<PullMessageResponseHeader>();
  assert(responseHeader != nullptr);

  return new PullResultExt(pullStatus, responseHeader->nextBeginOffset, responseHeader->minOffset,
                           responseHeader->maxOffset, (int)responseHeader->suggestWhichBrokerId, response->body());
}

MQMessageExt MQClientAPIImpl::viewMessage(const std::string& addr, int64_t phyoffset, int timeoutMillis) {
  auto* requestHeader = new ViewMessageRequestHeader();
  requestHeader->offset = phyoffset;

  RemotingCommand request(VIEW_MESSAGE_BY_ID, requestHeader);

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      // TODO: ...
    }
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

int64_t MQClientAPIImpl::searchOffset(const std::string& addr,
                                      const std::string& topic,
                                      int queueId,
                                      int64_t timestamp,
                                      int timeoutMillis) {
  auto* requestHeader = new SearchOffsetRequestHeader();
  requestHeader->topic = topic;
  requestHeader->queueId = queueId;
  requestHeader->timestamp = timestamp;

  RemotingCommand request(SEARCH_OFFSET_BY_TIMESTAMP, requestHeader);

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      auto* responseHeader = response->decodeCommandCustomHeader<SearchOffsetResponseHeader>();
      assert(responseHeader != nullptr);
      return responseHeader->offset;
    }
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

int64_t MQClientAPIImpl::getMaxOffset(const std::string& addr,
                                      const std::string& topic,
                                      int queueId,
                                      int timeoutMillis) {
  auto* requestHeader = new GetMaxOffsetRequestHeader();
  requestHeader->topic = topic;
  requestHeader->queueId = queueId;

  RemotingCommand request(GET_MAX_OFFSET, requestHeader);

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      auto* responseHeader = response->decodeCommandCustomHeader<GetMaxOffsetResponseHeader>();
      return responseHeader->offset;
    }
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

int64_t MQClientAPIImpl::getMinOffset(const std::string& addr,
                                      const std::string& topic,
                                      int queueId,
                                      int timeoutMillis) {
  auto* requestHeader = new GetMinOffsetRequestHeader();
  requestHeader->topic = topic;
  requestHeader->queueId = queueId;

  RemotingCommand request(GET_MIN_OFFSET, requestHeader);

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      auto* responseHeader = response->decodeCommandCustomHeader<GetMinOffsetResponseHeader>();
      assert(responseHeader != nullptr);
      return responseHeader->offset;
    }
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

int64_t MQClientAPIImpl::getEarliestMsgStoretime(const std::string& addr,
                                                 const std::string& topic,
                                                 int queueId,
                                                 int timeoutMillis) {
  auto* requestHeader = new GetEarliestMsgStoretimeRequestHeader();
  requestHeader->topic = topic;
  requestHeader->queueId = queueId;

  RemotingCommand request(GET_EARLIEST_MSG_STORETIME, requestHeader);

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      auto* responseHeader = response->decodeCommandCustomHeader<GetEarliestMsgStoretimeResponseHeader>();
      assert(responseHeader != nullptr);
      return responseHeader->timestamp;
    }
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

void MQClientAPIImpl::getConsumerIdListByGroup(const std::string& addr,
                                               const std::string& consumerGroup,
                                               std::vector<std::string>& cids,
                                               int timeoutMillis) {
  auto* requestHeader = new GetConsumerListByGroupRequestHeader();
  requestHeader->consumerGroup = consumerGroup;

  RemotingCommand request(GET_CONSUMER_LIST_BY_GROUP, requestHeader);

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      auto responseBody = response->body();
      if (responseBody != nullptr && responseBody->size() > 0) {
        std::unique_ptr<GetConsumerListByGroupResponseBody> body(
            GetConsumerListByGroupResponseBody::Decode(*responseBody));
        cids = std::move(body->consumerIdList);
        return;
      }
    }
    case SYSTEM_ERROR:
    // no consumer for this group
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

int64_t MQClientAPIImpl::queryConsumerOffset(const std::string& addr,
                                             QueryConsumerOffsetRequestHeader* requestHeader,
                                             int timeoutMillis) {
  RemotingCommand request(QUERY_CONSUMER_OFFSET, requestHeader);

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      auto* responseHeader = response->decodeCommandCustomHeader<QueryConsumerOffsetResponseHeader>();
      assert(responseHeader != nullptr);
      return responseHeader->offset;
    }
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

void MQClientAPIImpl::updateConsumerOffset(const std::string& addr,
                                           UpdateConsumerOffsetRequestHeader* requestHeader,
                                           int timeoutMillis) {
  RemotingCommand request(UPDATE_CONSUMER_OFFSET, requestHeader);

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      return;
    }
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

void MQClientAPIImpl::updateConsumerOffsetOneway(const std::string& addr,
                                                 UpdateConsumerOffsetRequestHeader* requestHeader,
                                                 int timeoutMillis) {
  RemotingCommand request(UPDATE_CONSUMER_OFFSET, requestHeader);

  remoting_client_->invokeOneway(addr, request);
}

void MQClientAPIImpl::sendHearbeat(const std::string& addr, HeartbeatData* heartbeatData, long timeoutMillis) {
  RemotingCommand request(HEART_BEAT, nullptr);
  request.set_body(heartbeatData->encode());

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      LOG_DEBUG_NEW("sendHeartbeat to broker:{} success", addr);
      return;
    }
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

void MQClientAPIImpl::unregisterClient(const std::string& addr,
                                       const std::string& clientID,
                                       const std::string& producerGroup,
                                       const std::string& consumerGroup) {
  LOG_INFO("unregisterClient to broker:%s", addr.c_str());
  RemotingCommand request(UNREGISTER_CLIENT, new UnregisterClientRequestHeader(clientID, producerGroup, consumerGroup));

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS:
      LOG_INFO("unregisterClient to:%s success", addr.c_str());
      return;
    default:
      break;
  }

  LOG_WARN("unregisterClient fail:%s, %d", response->remark().c_str(), response->code());
  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

void MQClientAPIImpl::endTransactionOneway(const std::string& addr,
                                           EndTransactionRequestHeader* requestHeader,
                                           const std::string& remark) {
  RemotingCommand request(END_TRANSACTION, requestHeader);
  request.set_remark(remark);

  remoting_client_->invokeOneway(addr, request);
}

void MQClientAPIImpl::consumerSendMessageBack(const std::string& addr,
                                              MessageExtPtr msg,
                                              const std::string& consumerGroup,
                                              int delayLevel,
                                              int timeoutMillis,
                                              int maxConsumeRetryTimes) {
  auto* requestHeader = new ConsumerSendMsgBackRequestHeader();
  requestHeader->group = consumerGroup;
  requestHeader->originTopic = msg->topic();
  requestHeader->offset = msg->commit_log_offset();
  requestHeader->delayLevel = delayLevel;
  requestHeader->originMsgId = msg->msg_id();
  requestHeader->maxReconsumeTimes = maxConsumeRetryTimes;

  RemotingCommand request(CONSUMER_SEND_MSG_BACK, requestHeader);

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      return;
    }
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

void MQClientAPIImpl::lockBatchMQ(const std::string& addr,
                                  LockBatchRequestBody* requestBody,
                                  std::vector<MQMessageQueue>& mqs,
                                  int timeoutMillis) {
  RemotingCommand request(LOCK_BATCH_MQ, nullptr);
  request.set_body(requestBody->encode());

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      auto requestBody = response->body();
      if (requestBody != nullptr && requestBody->size() > 0) {
        std::unique_ptr<LockBatchResponseBody> body(LockBatchResponseBody::Decode(*requestBody));
        mqs = std::move(body->lock_ok_mq_set());
      } else {
        mqs.clear();
      }
      return;
    } break;
    default:
      break;
  }

  THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
}

void MQClientAPIImpl::unlockBatchMQ(const std::string& addr,
                                    UnlockBatchRequestBody* requestBody,
                                    int timeoutMillis,
                                    bool oneway) {
  RemotingCommand request(UNLOCK_BATCH_MQ, nullptr);
  request.set_body(requestBody->encode());

  if (oneway) {
    remoting_client_->invokeOneway(addr, request);
  } else {
    std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(addr, request, timeoutMillis));
    assert(response != nullptr);
    switch (response->code()) {
      case SUCCESS: {
        return;
      } break;
      default:
        break;
    }

    THROW_MQEXCEPTION(MQBrokerException, response->remark(), response->code());
  }
}

TopicRouteData* MQClientAPIImpl::getTopicRouteInfoFromNameServer(const std::string& topic, int timeoutMillis) {
  RemotingCommand request(GET_ROUTEINFO_BY_TOPIC, new GetRouteInfoRequestHeader(topic));

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(null, request, timeoutMillis));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      auto responseBody = response->body();
      if (responseBody != nullptr && responseBody->size() > 0) {
        return TopicRouteData::Decode(*responseBody);
      }
    }
    case TOPIC_NOT_EXIST:
    default:
      break;
  }

  THROW_MQEXCEPTION(MQClientException, response->remark(), response->code());
}

TopicList* MQClientAPIImpl::getTopicListFromNameServer() {
  RemotingCommand request(GET_ALL_TOPIC_LIST_FROM_NAMESERVER, nullptr);

  std::unique_ptr<RemotingCommand> response(remoting_client_->invokeSync(null, request));
  assert(response != nullptr);
  switch (response->code()) {
    case SUCCESS: {
      auto responseBody = response->body();
      if (responseBody != nullptr && responseBody->size() > 0) {
        return TopicList::Decode(*responseBody);
      }
    }
    default:
      break;
  }

  THROW_MQEXCEPTION(MQClientException, response->remark(), response->code());
}

int MQClientAPIImpl::wipeWritePermOfBroker(const std::string& namesrvAddr,
                                           const std::string& brokerName,
                                           int timeoutMillis) {
  return 0;
}

void MQClientAPIImpl::deleteTopicInBroker(const std::string& addr, const std::string& topic, int timeoutMillis) {}

void MQClientAPIImpl::deleteTopicInNameServer(const std::string& addr, const std::string& topic, int timeoutMillis) {}

void MQClientAPIImpl::deleteSubscriptionGroup(const std::string& addr,
                                              const std::string& groupName,
                                              int timeoutMillis) {}

std::string MQClientAPIImpl::getKVConfigByValue(const std::string& projectNamespace,
                                                const std::string& projectGroup,
                                                int timeoutMillis) {
  return "";
}

void MQClientAPIImpl::deleteKVConfigByValue(const std::string& projectNamespace,
                                            const std::string& projectGroup,
                                            int timeoutMillis) {}

KVTable MQClientAPIImpl::getKVListByNamespace(const std::string& projectNamespace, int timeoutMillis) {
  return KVTable();
}

}  // namespace rocketmq
