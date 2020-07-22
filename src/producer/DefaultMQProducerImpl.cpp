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
#include "DefaultMQProducerImpl.h"

#include <cassert>
#include <typeindex>

#ifndef WIN32
#include <signal.h>
#endif

#include "ClientErrorCode.h"
#include "CommunicationMode.h"
#include "CorrelationIdUtil.hpp"
#include "Logging.h"
#include "MQClientAPIImpl.h"
#include "MQClientException.h"
#include "MQClientInstance.h"
#include "MQClientManager.h"
#include "MessageDecoder.h"
#include "MQFaultStrategy.h"
#include "MQProtos.h"
#include "MessageBatch.h"
#include "MessageClientIDSetter.h"
#include "MessageSysFlag.h"
#include "RequestFutureTable.h"
#include "TopicPublishInfo.h"
#include "TransactionMQProducer.h"
#include "Validators.h"
#include "protocol/header/CommandHeader.h"

namespace rocketmq {

class RequestSendCallback : public AutoDeleteSendCallback {
 public:
  RequestSendCallback(std::shared_ptr<RequestResponseFuture> requestFuture) : request_future_(requestFuture) {}

  void onSuccess(SendResult& sendResult) override { request_future_->setSendRequestOk(true); }

  void onException(MQException& e) noexcept override {
    request_future_->setSendRequestOk(false);
    request_future_->putResponseMessage(nullptr);
    request_future_->setCause(std::make_exception_ptr(e));
  }

 private:
  std::shared_ptr<RequestResponseFuture> request_future_;
};

class AsyncRequestSendCallback : public AutoDeleteSendCallback {
 public:
  AsyncRequestSendCallback(std::shared_ptr<RequestResponseFuture> requestFuture) : request_future_(requestFuture) {}

  void onSuccess(SendResult& sendResult) override { request_future_->setSendRequestOk(true); }

  void onException(MQException& e) noexcept override {
    request_future_->setCause(std::make_exception_ptr(e));
    auto response_future = RequestFutureTable::removeRequestFuture(request_future_->getCorrelationId());
    if (response_future != nullptr) {
      // response_future is same as request_future_
      response_future->setSendRequestOk(false);
      response_future->putResponseMessage(nullptr);
      try {
        response_future->executeRequestCallback();
      } catch (std::exception& e) {
        LOG_WARN_NEW("execute requestCallback in requestFail, and callback throw {}", e.what());
      }
    }
  }

 private:
  std::shared_ptr<RequestResponseFuture> request_future_;
};

DefaultMQProducerImpl::DefaultMQProducerImpl(DefaultMQProducerConfigPtr config)
    : DefaultMQProducerImpl(config, nullptr) {}

DefaultMQProducerImpl::DefaultMQProducerImpl(DefaultMQProducerConfigPtr config, RPCHookPtr rpcHook)
    : MQClientImpl(config, rpcHook),
      m_producerConfig(config),
      m_mqFaultStrategy(new MQFaultStrategy()),
      m_checkTransactionExecutor(nullptr) {}

DefaultMQProducerImpl::~DefaultMQProducerImpl() = default;

void DefaultMQProducerImpl::start() {
#ifndef WIN32
  /* Ignore the SIGPIPE */
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = 0;
  ::sigaction(SIGPIPE, &sa, 0);
#endif

  switch (m_serviceState) {
    case CREATE_JUST: {
      m_serviceState = START_FAILED;

      m_producerConfig->changeInstanceNameToPID();

      LOG_INFO_NEW("DefaultMQProducerImpl: {} start", m_producerConfig->getGroupName());

      MQClientImpl::start();

      bool registerOK = m_clientInstance->registerProducer(m_producerConfig->getGroupName(), this);
      if (!registerOK) {
        m_serviceState = CREATE_JUST;
        THROW_MQEXCEPTION(MQClientException, "The producer group[" + m_producerConfig->getGroupName() +
                                                 "] has been created before, specify another name please.",
                          -1);
      }

      m_clientInstance->start();
      LOG_INFO_NEW("the producer [{}] start OK.", m_producerConfig->getGroupName());
      m_serviceState = RUNNING;
      break;
    }
    case RUNNING:
    case START_FAILED:
    case SHUTDOWN_ALREADY:
      break;
    default:
      break;
  }

  m_clientInstance->sendHeartbeatToAllBrokerWithLock();
}

void DefaultMQProducerImpl::shutdown() {
  switch (m_serviceState) {
    case RUNNING: {
      LOG_INFO("DefaultMQProducerImpl shutdown");
      m_clientInstance->unregisterProducer(m_producerConfig->getGroupName());
      m_clientInstance->shutdown();

      m_serviceState = SHUTDOWN_ALREADY;
      break;
    }
    case SHUTDOWN_ALREADY:
    case CREATE_JUST:
      break;
    default:
      break;
  }
}

void DefaultMQProducerImpl::initTransactionEnv() {
  if (nullptr == m_checkTransactionExecutor) {
    m_checkTransactionExecutor.reset(new thread_pool_executor(1, false));
  }
  m_checkTransactionExecutor->startup();
}

void DefaultMQProducerImpl::destroyTransactionEnv() {
  m_checkTransactionExecutor->shutdown();
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg) {
  return send(msg, m_producerConfig->getSendMsgTimeout());
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, long timeout) {
  try {
    std::unique_ptr<SendResult> sendResult(
        sendDefaultImpl(msg.getMessageImpl(), CommunicationMode::SYNC, nullptr, timeout));
    return *sendResult;
  } catch (MQException& e) {
    LOG_ERROR_NEW("send failed, exception:{}", e.what());
    throw e;
  }
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, const MQMessageQueue& mq) {
  return send(msg, mq, m_producerConfig->getSendMsgTimeout());
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, const MQMessageQueue& mq, long timeout) {
  Validators::checkMessage(msg, m_producerConfig->getMaxMessageSize());

  if (msg.getTopic() != mq.getTopic()) {
    THROW_MQEXCEPTION(MQClientException, "message's topic not equal mq's topic", -1);
  }

  try {
    std::unique_ptr<SendResult> sendResult(
        sendKernelImpl(msg.getMessageImpl(), mq, CommunicationMode::SYNC, nullptr, nullptr, timeout));
    return *sendResult;
  } catch (MQException& e) {
    LOG_ERROR_NEW("send failed, exception:{}", e.what());
    throw e;
  }
}

void DefaultMQProducerImpl::send(MQMessage& msg, SendCallback* sendCallback) noexcept {
  return send(msg, sendCallback, m_producerConfig->getSendMsgTimeout());
}

void DefaultMQProducerImpl::send(MQMessage& msg, SendCallback* sendCallback, long timeout) noexcept {
  try {
    (void)sendDefaultImpl(msg.getMessageImpl(), CommunicationMode::ASYNC, sendCallback, timeout);
  } catch (MQException& e) {
    LOG_ERROR_NEW("send failed, exception:{}", e.what());
    sendCallback->onException(e);
    if (sendCallback->getSendCallbackType() == SEND_CALLBACK_TYPE_AUTO_DELETE) {
      deleteAndZero(sendCallback);
    }
  } catch (std::exception& e) {
    LOG_FATAL_NEW("[BUG] encounter unexcepted exception: {}", e.what());
    exit(-1);
  }
}

void DefaultMQProducerImpl::send(MQMessage& msg, const MQMessageQueue& mq, SendCallback* sendCallback) noexcept {
  return send(msg, mq, sendCallback, m_producerConfig->getSendMsgTimeout());
}

void DefaultMQProducerImpl::send(MQMessage& msg,
                                 const MQMessageQueue& mq,
                                 SendCallback* sendCallback,
                                 long timeout) noexcept {
  try {
    Validators::checkMessage(msg, m_producerConfig->getMaxMessageSize());

    if (msg.getTopic() != mq.getTopic()) {
      THROW_MQEXCEPTION(MQClientException, "message's topic not equal mq's topic", -1);
    }

    try {
      sendKernelImpl(msg.getMessageImpl(), mq, CommunicationMode::ASYNC, sendCallback, nullptr, timeout);
    } catch (MQBrokerException& e) {
      std::string info = std::string("unknown exception, ") + e.what();
      THROW_MQEXCEPTION(MQClientException, info, e.GetError());
    }
  } catch (MQException& e) {
    LOG_ERROR_NEW("send failed, exception:{}", e.what());
    sendCallback->onException(e);
    if (sendCallback->getSendCallbackType() == SEND_CALLBACK_TYPE_AUTO_DELETE) {
      deleteAndZero(sendCallback);
    }
  } catch (std::exception& e) {
    LOG_FATAL_NEW("[BUG] encounter unexcepted exception: {}", e.what());
    exit(-1);
  }
}

void DefaultMQProducerImpl::sendOneway(MQMessage& msg) {
  try {
    sendDefaultImpl(msg.getMessageImpl(), CommunicationMode::ONEWAY, nullptr, m_producerConfig->getSendMsgTimeout());
  } catch (MQBrokerException e) {
    std::string info = std::string("unknown exception, ") + e.what();
    THROW_MQEXCEPTION(MQClientException, info, e.GetError());
  }
}

void DefaultMQProducerImpl::sendOneway(MQMessage& msg, const MQMessageQueue& mq) {
  Validators::checkMessage(msg, m_producerConfig->getMaxMessageSize());

  if (msg.getTopic() != mq.getTopic()) {
    THROW_MQEXCEPTION(MQClientException, "message's topic not equal mq's topic", -1);
  }

  try {
    sendKernelImpl(msg.getMessageImpl(), mq, CommunicationMode::ONEWAY, nullptr, nullptr,
                   m_producerConfig->getSendMsgTimeout());
  } catch (MQBrokerException e) {
    std::string info = std::string("unknown exception, ") + e.what();
    THROW_MQEXCEPTION(MQClientException, info, e.GetError());
  }
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, MessageQueueSelector* selector, void* arg) {
  return send(msg, selector, arg, m_producerConfig->getSendMsgTimeout());
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, MessageQueueSelector* selector, void* arg, long timeout) {
  try {
    std::unique_ptr<SendResult> result(
        sendSelectImpl(msg.getMessageImpl(), selector, arg, CommunicationMode::SYNC, nullptr, timeout));
    return *result.get();
  } catch (MQException& e) {
    LOG_ERROR_NEW("send failed, exception:{}", e.what());
    throw e;
  }
}

void DefaultMQProducerImpl::send(MQMessage& msg,
                                 MessageQueueSelector* selector,
                                 void* arg,
                                 SendCallback* sendCallback) noexcept {
  return send(msg, selector, arg, sendCallback, m_producerConfig->getSendMsgTimeout());
}

void DefaultMQProducerImpl::send(MQMessage& msg,
                                 MessageQueueSelector* selector,
                                 void* arg,
                                 SendCallback* sendCallback,
                                 long timeout) noexcept {
  try {
    try {
      sendSelectImpl(msg.getMessageImpl(), selector, arg, CommunicationMode::ASYNC, sendCallback, timeout);
    } catch (MQBrokerException& e) {
      std::string info = std::string("unknown exception, ") + e.what();
      THROW_MQEXCEPTION(MQClientException, info, e.GetError());
    }
  } catch (MQException& e) {
    LOG_ERROR_NEW("send failed, exception:{}", e.what());
    sendCallback->onException(e);
    if (sendCallback->getSendCallbackType() == SEND_CALLBACK_TYPE_AUTO_DELETE) {
      deleteAndZero(sendCallback);
    }
  } catch (std::exception& e) {
    LOG_FATAL_NEW("[BUG] encounter unexcepted exception: {}", e.what());
    exit(-1);
  }
}

void DefaultMQProducerImpl::sendOneway(MQMessage& msg, MessageQueueSelector* selector, void* arg) {
  try {
    sendSelectImpl(msg.getMessageImpl(), selector, arg, CommunicationMode::ONEWAY, nullptr,
                   m_producerConfig->getSendMsgTimeout());
  } catch (MQBrokerException e) {
    std::string info = std::string("unknown exception, ") + e.what();
    THROW_MQEXCEPTION(MQClientException, info, e.GetError());
  }
}

TransactionSendResult DefaultMQProducerImpl::sendMessageInTransaction(MQMessage& msg, void* arg) {
  try {
    std::unique_ptr<TransactionSendResult> sendResult(
        sendMessageInTransactionImpl(msg.getMessageImpl(), arg, m_producerConfig->getSendMsgTimeout()));
    return *sendResult;
  } catch (MQException& e) {
    LOG_ERROR_NEW("sendMessageInTransaction failed, exception:{}", e.what());
    throw e;
  }
}

SendResult DefaultMQProducerImpl::send(std::vector<MQMessage>& msgs) {
  MQMessage batchMessage(batch(msgs));
  return send(batchMessage);
}

SendResult DefaultMQProducerImpl::send(std::vector<MQMessage>& msgs, long timeout) {
  MQMessage batchMessage(batch(msgs));
  return send(batchMessage, timeout);
}

SendResult DefaultMQProducerImpl::send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq) {
  MQMessage batchMessage(batch(msgs));
  return send(batchMessage, mq);
}

SendResult DefaultMQProducerImpl::send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq, long timeout) {
  MQMessage batchMessage(batch(msgs));
  return send(batchMessage, mq, timeout);
}

MessagePtr DefaultMQProducerImpl::batch(std::vector<MQMessage>& msgs) {
  if (msgs.size() < 1) {
    THROW_MQEXCEPTION(MQClientException, "msgs need one message at least", -1);
  }

  try {
    auto messageBatch = MessageBatch::generateFromList(msgs);
    for (auto& message : messageBatch->getMessages()) {
      Validators::checkMessage(message, m_producerConfig->getMaxMessageSize());
      MessageClientIDSetter::setUniqID(const_cast<MQMessage&>(message));
    }
    messageBatch->setBody(messageBatch->encode());
    return messageBatch;
  } catch (std::exception& e) {
    THROW_MQEXCEPTION(MQClientException, "Failed to initiate the MessageBatch", -1);
  }
}

MQMessage DefaultMQProducerImpl::request(MQMessage& msg, long timeout) {
  auto beginTimestamp = UtilAll::currentTimeMillis();
  prepareSendRequest(msg, timeout);
  const auto& correlationId = msg.getProperty(MQMessageConst::PROPERTY_CORRELATION_ID);

  std::exception_ptr eptr = nullptr;
  MessagePtr responseMessage;
  try {
    auto requestResponseFuture = std::make_shared<RequestResponseFuture>(correlationId, timeout, nullptr);
    RequestFutureTable::putRequestFuture(correlationId, requestResponseFuture);

    auto cost = UtilAll::currentTimeMillis() - beginTimestamp;
    sendDefaultImpl(msg.getMessageImpl(), CommunicationMode::ASYNC, new RequestSendCallback(requestResponseFuture),
                    timeout - cost);

    responseMessage = requestResponseFuture->waitResponseMessage(timeout - cost);
    if (responseMessage == nullptr) {
      if (requestResponseFuture->isSendRequestOk()) {
        std::string info = "send request message to <" + msg.getTopic() + "> OK, but wait reply message timeout, " +
                           UtilAll::to_string(timeout) + " ms.";
        THROW_MQEXCEPTION(RequestTimeoutException, info, ClientErrorCode::REQUEST_TIMEOUT_EXCEPTION);
      } else {
        std::string info = "send request message to <" + msg.getTopic() + "> fail";
        THROW_MQEXCEPTION2(MQClientException, info, -1, requestResponseFuture->getCause());
      }
    }
  } catch (...) {
    eptr = std::current_exception();
  }

  // finally
  RequestFutureTable::removeRequestFuture(correlationId);

  if (eptr != nullptr) {
    std::rethrow_exception(eptr);
  }

  return MQMessage(responseMessage);
}

void DefaultMQProducerImpl::request(MQMessage& msg, RequestCallback* requestCallback, long timeout) {
  auto beginTimestamp = UtilAll::currentTimeMillis();
  prepareSendRequest(msg, timeout);
  const auto& correlationId = msg.getProperty(MQMessageConst::PROPERTY_CORRELATION_ID);

  auto requestResponseFuture = std::make_shared<RequestResponseFuture>(correlationId, timeout, requestCallback);
  RequestFutureTable::putRequestFuture(correlationId, requestResponseFuture);

  auto cost = UtilAll::currentTimeMillis() - beginTimestamp;
  sendDefaultImpl(msg.getMessageImpl(), CommunicationMode::ASYNC, new AsyncRequestSendCallback(requestResponseFuture),
                  timeout - cost);
}

MQMessage DefaultMQProducerImpl::request(MQMessage& msg, const MQMessageQueue& mq, long timeout) {
  auto beginTimestamp = UtilAll::currentTimeMillis();
  prepareSendRequest(msg, timeout);
  const auto& correlationId = msg.getProperty(MQMessageConst::PROPERTY_CORRELATION_ID);

  std::exception_ptr eptr = nullptr;
  MessagePtr responseMessage;
  try {
    auto requestResponseFuture = std::make_shared<RequestResponseFuture>(correlationId, timeout, nullptr);
    RequestFutureTable::putRequestFuture(correlationId, requestResponseFuture);

    auto cost = UtilAll::currentTimeMillis() - beginTimestamp;
    sendKernelImpl(msg.getMessageImpl(), mq, CommunicationMode::ASYNC, new RequestSendCallback(requestResponseFuture),
                   nullptr, timeout - cost);

    responseMessage = requestResponseFuture->waitResponseMessage(timeout - cost);
    if (responseMessage == nullptr) {
      if (requestResponseFuture->isSendRequestOk()) {
        std::string info = "send request message to <" + msg.getTopic() + "> OK, but wait reply message timeout, " +
                           UtilAll::to_string(timeout) + " ms.";
        THROW_MQEXCEPTION(RequestTimeoutException, info, ClientErrorCode::REQUEST_TIMEOUT_EXCEPTION);
      } else {
        std::string info = "send request message to <" + msg.getTopic() + "> fail";
        THROW_MQEXCEPTION2(MQClientException, info, -1, requestResponseFuture->getCause());
      }
    }
  } catch (...) {
    eptr = std::current_exception();
  }

  // finally
  RequestFutureTable::removeRequestFuture(correlationId);

  if (eptr != nullptr) {
    std::rethrow_exception(eptr);
  }

  return MQMessage(responseMessage);
}

void DefaultMQProducerImpl::request(MQMessage& msg,
                                    const MQMessageQueue& mq,
                                    RequestCallback* requestCallback,
                                    long timeout) {
  auto beginTimestamp = UtilAll::currentTimeMillis();
  prepareSendRequest(msg, timeout);
  const auto& correlationId = msg.getProperty(MQMessageConst::PROPERTY_CORRELATION_ID);

  auto requestResponseFuture = std::make_shared<RequestResponseFuture>(correlationId, timeout, requestCallback);
  RequestFutureTable::putRequestFuture(correlationId, requestResponseFuture);

  auto cost = UtilAll::currentTimeMillis() - beginTimestamp;
  sendKernelImpl(msg.getMessageImpl(), mq, CommunicationMode::ASYNC,
                 new AsyncRequestSendCallback(requestResponseFuture), nullptr, timeout - cost);
}

MQMessage DefaultMQProducerImpl::request(MQMessage& msg, MessageQueueSelector* selector, void* arg, long timeout) {
  auto beginTimestamp = UtilAll::currentTimeMillis();
  prepareSendRequest(msg, timeout);
  const auto& correlationId = msg.getProperty(MQMessageConst::PROPERTY_CORRELATION_ID);

  std::exception_ptr eptr = nullptr;
  MessagePtr responseMessage;
  try {
    auto requestResponseFuture = std::make_shared<RequestResponseFuture>(correlationId, timeout, nullptr);
    RequestFutureTable::putRequestFuture(correlationId, requestResponseFuture);

    auto cost = UtilAll::currentTimeMillis() - beginTimestamp;
    sendSelectImpl(msg.getMessageImpl(), selector, arg, CommunicationMode::ASYNC,
                   new RequestSendCallback(requestResponseFuture), timeout - cost);

    responseMessage = requestResponseFuture->waitResponseMessage(timeout - cost);
    if (responseMessage == nullptr) {
      if (requestResponseFuture->isSendRequestOk()) {
        std::string info = "send request message to <" + msg.getTopic() + "> OK, but wait reply message timeout, " +
                           UtilAll::to_string(timeout) + " ms.";
        THROW_MQEXCEPTION(RequestTimeoutException, info, ClientErrorCode::REQUEST_TIMEOUT_EXCEPTION);
      } else {
        std::string info = "send request message to <" + msg.getTopic() + "> fail";
        THROW_MQEXCEPTION2(MQClientException, info, -1, requestResponseFuture->getCause());
      }
    }
  } catch (...) {
    eptr = std::current_exception();
  }

  // finally
  RequestFutureTable::removeRequestFuture(correlationId);

  if (eptr != nullptr) {
    std::rethrow_exception(eptr);
  }

  return MQMessage(responseMessage);
}

void DefaultMQProducerImpl::request(MQMessage& msg,
                                    MessageQueueSelector* selector,
                                    void* arg,
                                    RequestCallback* requestCallback,
                                    long timeout) {
  auto beginTimestamp = UtilAll::currentTimeMillis();
  prepareSendRequest(msg, timeout);
  const auto& correlationId = msg.getProperty(MQMessageConst::PROPERTY_CORRELATION_ID);

  auto requestResponseFuture = std::make_shared<RequestResponseFuture>(correlationId, timeout, requestCallback);
  RequestFutureTable::putRequestFuture(correlationId, requestResponseFuture);

  auto cost = UtilAll::currentTimeMillis() - beginTimestamp;
  sendSelectImpl(msg.getMessageImpl(), selector, arg, CommunicationMode::ASYNC,
                 new AsyncRequestSendCallback(requestResponseFuture), timeout - cost);
}

void DefaultMQProducerImpl::prepareSendRequest(Message& msg, long timeout) {
  const auto correlationId = CorrelationIdUtil::createCorrelationId();
  const auto& requestClientId = m_clientInstance->getClientId();
  MessageAccessor::putProperty(msg, MQMessageConst::PROPERTY_CORRELATION_ID, correlationId);
  MessageAccessor::putProperty(msg, MQMessageConst::PROPERTY_MESSAGE_REPLY_TO_CLIENT, requestClientId);
  MessageAccessor::putProperty(msg, MQMessageConst::PROPERTY_MESSAGE_TTL, UtilAll::to_string(timeout));

  auto hasRouteData = m_clientInstance->getTopicRouteData(msg.getTopic()) != nullptr;
  if (!hasRouteData) {
    auto beginTimestamp = UtilAll::currentTimeMillis();
    m_clientInstance->tryToFindTopicPublishInfo(msg.getTopic());
    m_clientInstance->sendHeartbeatToAllBrokerWithLock();
    auto cost = UtilAll::currentTimeMillis() - beginTimestamp;
    if (cost > 500) {
      LOG_WARN_NEW("prepare send request for <{}> cost {} ms", msg.getTopic(), cost);
    }
  }
}

const MQMessageQueue& DefaultMQProducerImpl::selectOneMessageQueue(const TopicPublishInfo* tpInfo,
                                                                   const std::string& lastBrokerName) {
  return m_mqFaultStrategy->selectOneMessageQueue(tpInfo, lastBrokerName);
}

void DefaultMQProducerImpl::updateFaultItem(const std::string& brokerName, const long currentLatency, bool isolation) {
  m_mqFaultStrategy->updateFaultItem(brokerName, currentLatency, isolation);
}

SendResult* DefaultMQProducerImpl::sendDefaultImpl(MessagePtr msg,
                                                   CommunicationMode communicationMode,
                                                   SendCallback* sendCallback,
                                                   long timeout) {
  Validators::checkMessage(*msg, m_producerConfig->getMaxMessageSize());

  uint64_t beginTimestampFirst = UtilAll::currentTimeMillis();
  uint64_t beginTimestampPrev = beginTimestampFirst;
  uint64_t endTimestamp = beginTimestampFirst;
  auto topicPublishInfo = m_clientInstance->tryToFindTopicPublishInfo(msg->getTopic());
  if (topicPublishInfo != nullptr && topicPublishInfo->ok()) {
    bool callTimeout = false;
    std::unique_ptr<SendResult> sendResult;
    int timesTotal = communicationMode == CommunicationMode::SYNC ? 1 + m_producerConfig->getRetryTimes() : 1;
    int times = 0;
    std::string lastBrokerName;
    for (; times < timesTotal; times++) {
      const auto& mq = selectOneMessageQueue(topicPublishInfo.get(), lastBrokerName);
      lastBrokerName = mq.getBrokerName();

      try {
        LOG_DEBUG_NEW("send to mq: {}", mq.toString());

        beginTimestampPrev = UtilAll::currentTimeMillis();
        if (times > 0) {
          // TODO: Reset topic with namespace during resend.
        }
        long costTime = beginTimestampPrev - beginTimestampFirst;
        if (timeout < costTime) {
          callTimeout = true;
          break;
        }

        sendResult.reset(
            sendKernelImpl(msg, mq, communicationMode, sendCallback, topicPublishInfo, timeout - costTime));
        endTimestamp = UtilAll::currentTimeMillis();
        updateFaultItem(mq.getBrokerName(), endTimestamp - beginTimestampPrev, false);
        switch (communicationMode) {
          case CommunicationMode::ASYNC:
            return nullptr;
          case CommunicationMode::ONEWAY:
            return nullptr;
          case CommunicationMode::SYNC:
            if (sendResult->getSendStatus() != SEND_OK) {
              if (m_producerConfig->isRetryAnotherBrokerWhenNotStoreOK()) {
                continue;
              }
            }

            return sendResult.release();
          default:
            break;
        }
      } catch (const std::exception& e) {
        // TODO: 区分异常类型
        endTimestamp = UtilAll::currentTimeMillis();
        updateFaultItem(mq.getBrokerName(), endTimestamp - beginTimestampPrev, true);
        LOG_WARN_NEW("send failed of times:{}, brokerName:{}. exception:{}", times, mq.getBrokerName(), e.what());
        continue;
      }

    }  // end of for

    if (sendResult != nullptr) {
      return sendResult.release();
    }

    std::string info = "Send [" + UtilAll::to_string(times) + "] times, still failed, cost [" +
                       UtilAll::to_string(UtilAll::currentTimeMillis() - beginTimestampFirst) + "]ms, Topic: " +
                       msg->getTopic();
    THROW_MQEXCEPTION(MQClientException, info, -1);
  }

  THROW_MQEXCEPTION(MQClientException, "No route info of this topic: " + msg->getTopic(), -1);
}

SendResult* DefaultMQProducerImpl::sendKernelImpl(MessagePtr msg,
                                                  const MQMessageQueue& mq,
                                                  CommunicationMode communicationMode,
                                                  SendCallback* sendCallback,
                                                  TopicPublishInfoPtr topicPublishInfo,
                                                  long timeout) {
  uint64_t beginStartTime = UtilAll::currentTimeMillis();
  std::string brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  if (brokerAddr.empty()) {
    m_clientInstance->tryToFindTopicPublishInfo(mq.getTopic());
    brokerAddr = m_clientInstance->findBrokerAddressInPublish(mq.getBrokerName());
  }

  if (!brokerAddr.empty()) {
    try {
      // for MessageBatch, ID has been set in the generating process
      if (!msg->isBatch()) {
        // msgId is produced by client, offsetMsgId produced by broker. (same with java sdk)
        MessageClientIDSetter::setUniqID(*msg);
      }

      int sysFlag = 0;
      bool msgBodyCompressed = false;
      if (tryToCompressMessage(*msg)) {
        sysFlag |= MessageSysFlag::COMPRESSED_FLAG;
        msgBodyCompressed = true;
      }

      const auto& tranMsg = msg->getProperty(MQMessageConst::PROPERTY_TRANSACTION_PREPARED);
      if (UtilAll::stob(tranMsg)) {
        sysFlag |= MessageSysFlag::TRANSACTION_PREPARED_TYPE;
      }

      // TOOD: send message hook

      std::unique_ptr<SendMessageRequestHeader> requestHeader(new SendMessageRequestHeader());
      requestHeader->producerGroup = m_producerConfig->getGroupName();
      requestHeader->topic = msg->getTopic();
      requestHeader->defaultTopic = AUTO_CREATE_TOPIC_KEY_TOPIC;
      requestHeader->defaultTopicQueueNums = 4;
      requestHeader->queueId = mq.getQueueId();
      requestHeader->sysFlag = sysFlag;
      requestHeader->bornTimestamp = UtilAll::currentTimeMillis();
      requestHeader->flag = msg->getFlag();
      requestHeader->properties = MessageDecoder::messageProperties2String(msg->getProperties());
      requestHeader->reconsumeTimes = 0;
      requestHeader->unitMode = false;
      requestHeader->batch = msg->isBatch();

      if (UtilAll::isRetryTopic(mq.getTopic())) {
        const auto& reconsumeTimes = MessageAccessor::getReconsumeTime(*msg);
        if (!reconsumeTimes.empty()) {
          requestHeader->reconsumeTimes = std::stoi(reconsumeTimes);
          MessageAccessor::clearProperty(*msg, MQMessageConst::PROPERTY_RECONSUME_TIME);
        }

        const auto& maxReconsumeTimes = MessageAccessor::getMaxReconsumeTimes(*msg);
        if (!maxReconsumeTimes.empty()) {
          requestHeader->maxReconsumeTimes = std::stoi(maxReconsumeTimes);
          MessageAccessor::clearProperty(*msg, MQMessageConst::PROPERTY_MAX_RECONSUME_TIMES);
        }
      }

      SendResult* sendResult = nullptr;
      switch (communicationMode) {
        case CommunicationMode::ASYNC: {
          long costTimeAsync = UtilAll::currentTimeMillis() - beginStartTime;
          if (timeout < costTimeAsync) {
            THROW_MQEXCEPTION(RemotingTooMuchRequestException, "sendKernelImpl call timeout", -1);
          }
          sendResult = m_clientInstance->getMQClientAPIImpl()->sendMessage(
              brokerAddr, mq.getBrokerName(), msg, std::move(requestHeader), timeout, communicationMode, sendCallback,
              topicPublishInfo, m_clientInstance, m_producerConfig->getRetryTimes4Async(), shared_from_this());
        } break;
        case CommunicationMode::ONEWAY:
        case CommunicationMode::SYNC: {
          long costTimeSync = UtilAll::currentTimeMillis() - beginStartTime;
          if (timeout < costTimeSync) {
            THROW_MQEXCEPTION(RemotingTooMuchRequestException, "sendKernelImpl call timeout", -1);
          }
          sendResult = m_clientInstance->getMQClientAPIImpl()->sendMessage(brokerAddr, mq.getBrokerName(), msg,
                                                                           std::move(requestHeader), timeout,
                                                                           communicationMode, shared_from_this());
        } break;
        default:
          assert(false);
          break;
      }

      return sendResult;
    } catch (MQException& e) {
      throw e;
    }
  }

  THROW_MQEXCEPTION(MQClientException, "The broker[" + mq.getBrokerName() + "] not exist", -1);
}

SendResult* DefaultMQProducerImpl::sendSelectImpl(MessagePtr msg,
                                                  MessageQueueSelector* selector,
                                                  void* arg,
                                                  CommunicationMode communicationMode,
                                                  SendCallback* sendCallback,
                                                  long timeout) {
  auto beginStartTime = UtilAll::currentTimeMillis();
  Validators::checkMessage(*msg, m_producerConfig->getMaxMessageSize());

  TopicPublishInfoPtr topicPublishInfo = m_clientInstance->tryToFindTopicPublishInfo(msg->getTopic());
  if (topicPublishInfo != nullptr && topicPublishInfo->ok()) {
    MQMessageQueue mq = selector->select(topicPublishInfo->getMessageQueueList(), MQMessage(msg), arg);

    auto costTime = UtilAll::currentTimeMillis() - beginStartTime;
    if (timeout < costTime) {
      THROW_MQEXCEPTION(RemotingTooMuchRequestException, "sendSelectImpl call timeout", -1);
    }

    return sendKernelImpl(msg, mq, communicationMode, sendCallback, nullptr, timeout - costTime);
  }

  std::string info = std::string("No route info for this topic, ") + msg->getTopic();
  THROW_MQEXCEPTION(MQClientException, info, -1);
}

TransactionSendResult* DefaultMQProducerImpl::sendMessageInTransactionImpl(MessagePtr msg, void* arg, long timeout) {
  auto* transactionListener = getCheckListener();
  if (nullptr == transactionListener) {
    THROW_MQEXCEPTION(MQClientException, "transactionListener is null", -1);
  }

  std::unique_ptr<SendResult> sendResult;
  MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_TRANSACTION_PREPARED, "true");
  MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_PRODUCER_GROUP, m_producerConfig->getGroupName());
  try {
    sendResult.reset(sendDefaultImpl(msg, CommunicationMode::SYNC, nullptr, timeout));
  } catch (MQException& e) {
    THROW_MQEXCEPTION(MQClientException, "send message Exception", -1);
  }

  LocalTransactionState localTransactionState = LocalTransactionState::UNKNOWN;
  std::exception_ptr localException;
  switch (sendResult->getSendStatus()) {
    case SendStatus::SEND_OK:
      try {
        if (!sendResult->getTransactionId().empty()) {
          msg->putProperty("__transactionId__", sendResult->getTransactionId());
        }
        const auto& transactionId = msg->getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
        if (!transactionId.empty()) {
          msg->setTransactionId(transactionId);
        }
        localTransactionState = transactionListener->executeLocalTransaction(MQMessage(msg), arg);
        if (localTransactionState != LocalTransactionState::COMMIT_MESSAGE) {
          LOG_INFO_NEW("executeLocalTransaction return not COMMIT_MESSAGE, msg:{}", msg->toString());
        }
      } catch (MQException& e) {
        LOG_INFO_NEW("executeLocalTransaction exception, msg:{}", msg->toString());
        localException = std::current_exception();
      }
      break;
    case SendStatus::SEND_FLUSH_DISK_TIMEOUT:
    case SendStatus::SEND_FLUSH_SLAVE_TIMEOUT:
    case SendStatus::SEND_SLAVE_NOT_AVAILABLE:
      localTransactionState = LocalTransactionState::ROLLBACK_MESSAGE;
      LOG_WARN_NEW("sendMessageInTransaction, send not ok, rollback, result:{}", sendResult->toString());
      break;
    default:
      break;
  }

  try {
    endTransaction(*sendResult, localTransactionState, localException);
  } catch (MQException& e) {
    LOG_WARN_NEW("local transaction execute {}, but end broker transaction failed: {}", localTransactionState,
                 e.what());
  }

  // FIXME: setTransactionId will cause OOM?
  TransactionSendResult* transactionSendResult = new TransactionSendResult(*sendResult.get());
  transactionSendResult->setTransactionId(msg->getTransactionId());
  transactionSendResult->setLocalTransactionState(localTransactionState);
  return transactionSendResult;
}

TransactionListener* DefaultMQProducerImpl::getCheckListener() {
  auto transactionProducerConfig = std::dynamic_pointer_cast<TransactionMQProducerConfig>(m_producerConfig);
  if (transactionProducerConfig != nullptr) {
    return transactionProducerConfig->getTransactionListener();
  }
  return nullptr;
};

void DefaultMQProducerImpl::checkTransactionState(const std::string& addr,
                                                  MessageExtPtr msg,
                                                  CheckTransactionStateRequestHeader* checkRequestHeader) {
  long tranStateTableOffset = checkRequestHeader->tranStateTableOffset;
  long commitLogOffset = checkRequestHeader->commitLogOffset;
  std::string msgId = checkRequestHeader->msgId;
  std::string transactionId = checkRequestHeader->transactionId;
  std::string offsetMsgId = checkRequestHeader->offsetMsgId;

  m_checkTransactionExecutor->submit(std::bind(&DefaultMQProducerImpl::checkTransactionStateImpl, this, addr, msg,
                                               tranStateTableOffset, commitLogOffset, msgId, transactionId,
                                               offsetMsgId));
}

void DefaultMQProducerImpl::checkTransactionStateImpl(const std::string& addr,
                                                      MessageExtPtr message,
                                                      long tranStateTableOffset,
                                                      long commitLogOffset,
                                                      const std::string& msgId,
                                                      const std::string& transactionId,
                                                      const std::string& offsetMsgId) {
  auto* transactionCheckListener = getCheckListener();
  if (nullptr == transactionCheckListener) {
    LOG_WARN_NEW("CheckTransactionState, pick transactionCheckListener by group[{}] failed",
                 m_producerConfig->getGroupName());
    return;
  }

  LocalTransactionState localTransactionState = UNKNOWN;
  std::exception_ptr exception = nullptr;
  try {
    localTransactionState = transactionCheckListener->checkLocalTransaction(MQMessageExt(message));
  } catch (MQException& e) {
    LOG_ERROR_NEW("Broker call checkTransactionState, but checkLocalTransactionState exception, {}", e.what());
    exception = std::current_exception();
  }

  EndTransactionRequestHeader* endHeader = new EndTransactionRequestHeader();
  endHeader->commitLogOffset = commitLogOffset;
  endHeader->producerGroup = m_producerConfig->getGroupName();
  endHeader->tranStateTableOffset = tranStateTableOffset;
  endHeader->fromTransactionCheck = true;

  std::string uniqueKey = message->getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
  if (uniqueKey.empty()) {
    uniqueKey = message->getMsgId();
  }

  endHeader->msgId = uniqueKey;
  endHeader->transactionId = transactionId;
  switch (localTransactionState) {
    case COMMIT_MESSAGE:
      endHeader->commitOrRollback = MessageSysFlag::TRANSACTION_COMMIT_TYPE;
      break;
    case ROLLBACK_MESSAGE:
      endHeader->commitOrRollback = MessageSysFlag::TRANSACTION_ROLLBACK_TYPE;
      LOG_WARN_NEW("when broker check, client rollback this transaction, {}", endHeader->toString());
      break;
    case UNKNOWN:
      endHeader->commitOrRollback = MessageSysFlag::TRANSACTION_NOT_TYPE;
      LOG_WARN_NEW("when broker check, client does not know this transaction state, {}", endHeader->toString());
      break;
    default:
      break;
  }

  std::string remark;
  if (exception != nullptr) {
    remark = "checkLocalTransactionState Exception: " + UtilAll::to_string(exception);
  }

  try {
    m_clientInstance->getMQClientAPIImpl()->endTransactionOneway(addr, endHeader, remark);
  } catch (std::exception& e) {
    LOG_ERROR_NEW("endTransactionOneway exception: {}", e.what());
  }
}

void DefaultMQProducerImpl::endTransaction(SendResult& sendResult,
                                           LocalTransactionState localTransactionState,
                                           std::exception_ptr& localException) {
  const auto& msg_id = !sendResult.getOffsetMsgId().empty() ? sendResult.getOffsetMsgId() : sendResult.getMsgId();
  auto id = MessageDecoder::decodeMessageId(msg_id);
  const auto& transactionId = sendResult.getTransactionId();
  std::string brokerAddr = m_clientInstance->findBrokerAddressInPublish(sendResult.getMessageQueue().getBrokerName());
  EndTransactionRequestHeader* requestHeader = new EndTransactionRequestHeader();
  requestHeader->transactionId = transactionId;
  requestHeader->commitLogOffset = id.getOffset();
  switch (localTransactionState) {
    case COMMIT_MESSAGE:
      requestHeader->commitOrRollback = MessageSysFlag::TRANSACTION_COMMIT_TYPE;
      break;
    case ROLLBACK_MESSAGE:
      requestHeader->commitOrRollback = MessageSysFlag::TRANSACTION_ROLLBACK_TYPE;
      break;
    case UNKNOWN:
      requestHeader->commitOrRollback = MessageSysFlag::TRANSACTION_NOT_TYPE;
      break;
    default:
      break;
  }

  requestHeader->producerGroup = m_producerConfig->getGroupName();
  requestHeader->tranStateTableOffset = sendResult.getQueueOffset();
  requestHeader->msgId = sendResult.getMsgId();

  std::string remark =
      localException ? ("executeLocalTransactionBranch exception: " + UtilAll::to_string(localException)) : null;

  m_clientInstance->getMQClientAPIImpl()->endTransactionOneway(brokerAddr, requestHeader, remark);
}

bool DefaultMQProducerImpl::tryToCompressMessage(Message& msg) {
  if (msg.isBatch()) {
    // batch dose not support compressing right now
    return false;
  }

  // already compressed
  if (UtilAll::stob(msg.getProperty(MQMessageConst::PROPERTY_ALREADY_COMPRESSED_FLAG))) {
    return true;
  }

  const auto& body = msg.getBody();
  if (body.size() >= m_producerConfig->getCompressMsgBodyOverHowmuch()) {
    std::string out_body;
    if (UtilAll::deflate(body, out_body, m_producerConfig->getCompressLevel())) {
      msg.setBody(std::move(out_body));
      msg.putProperty(MQMessageConst::PROPERTY_ALREADY_COMPRESSED_FLAG, "true");
      return true;
    }
  }

  return false;
}

bool DefaultMQProducerImpl::isSendLatencyFaultEnable() const {
  return m_mqFaultStrategy->isSendLatencyFaultEnable();
}

void DefaultMQProducerImpl::setSendLatencyFaultEnable(bool sendLatencyFaultEnable) {
  m_mqFaultStrategy->setSendLatencyFaultEnable(sendLatencyFaultEnable);
}

}  // namespace rocketmq
