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
#include "MQClientInstance.h"
#include "MQClientManager.h"
#include "MQException.h"
#include "MQFaultStrategy.h"
#include "MQMessageQueue.h"
#include "MQProtos.h"
#include "MessageBatch.h"
#include "MessageClientIDSetter.h"
#include "MessageDecoder.h"
#include "MessageSysFlag.h"
#include "RequestFutureTable.h"
#include "TopicPublishInfo.hpp"
#include "TransactionMQProducer.h"
#include "UtilAll.h"
#include "Validators.h"
#include "protocol/header/CommandHeader.h"

namespace rocketmq {

class RequestSendCallback : public AutoDeleteSendCallback {
 public:
  RequestSendCallback(std::shared_ptr<RequestResponseFuture> requestFuture) : request_future_(requestFuture) {}

  void onSuccess(SendResult& sendResult) override { request_future_->set_send_request_ok(true); }

  void onException(MQException& e) noexcept override {
    request_future_->set_send_request_ok(false);
    request_future_->putResponseMessage(nullptr);
    request_future_->set_cause(std::make_exception_ptr(e));
  }

 private:
  std::shared_ptr<RequestResponseFuture> request_future_;
};

class AsyncRequestSendCallback : public AutoDeleteSendCallback {
 public:
  AsyncRequestSendCallback(std::shared_ptr<RequestResponseFuture> requestFuture) : request_future_(requestFuture) {}

  void onSuccess(SendResult& sendResult) override { request_future_->set_send_request_ok(true); }

  void onException(MQException& e) noexcept override {
    request_future_->set_cause(std::make_exception_ptr(e));
    auto response_future = RequestFutureTable::removeRequestFuture(request_future_->correlation_id());
    if (response_future != nullptr) {
      // response_future is same as request_future_
      response_future->set_send_request_ok(false);
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
      mq_fault_strategy_(new MQFaultStrategy()),
      async_send_executor_(nullptr),
      check_transaction_executor_(nullptr) {}

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

  switch (service_state_) {
    case CREATE_JUST: {
      LOG_INFO_NEW("DefaultMQProducerImpl: {} start", client_config_->group_name());

      service_state_ = START_FAILED;

      client_config_->changeInstanceNameToPID();

      MQClientImpl::start();

      bool registerOK = client_instance_->registerProducer(
          dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->group_name(), this);
      if (!registerOK) {
        service_state_ = CREATE_JUST;
        THROW_MQEXCEPTION(MQClientException,
                          "The producer group[" + client_config_->group_name() +
                              "] has been created before, specify another name please.",
                          -1);
      }

      if (nullptr == async_send_executor_) {
        async_send_executor_.reset(new thread_pool_executor(
            "AsyncSendThread", dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->async_send_thread_nums(),
            false));
      }
      async_send_executor_->startup();

      client_instance_->start();

      LOG_INFO_NEW("the producer [{}] start OK.", client_config_->group_name());
      service_state_ = RUNNING;
      break;
    }
    case RUNNING:
    case START_FAILED:
    case SHUTDOWN_ALREADY:
      THROW_MQEXCEPTION(MQClientException, "The producer service state not OK, maybe started once", -1);
      break;
    default:
      break;
  }

  client_instance_->sendHeartbeatToAllBrokerWithLock();
}

void DefaultMQProducerImpl::shutdown() {
  switch (service_state_) {
    case RUNNING: {
      LOG_INFO("DefaultMQProducerImpl shutdown");

      async_send_executor_->shutdown();

      client_instance_->unregisterProducer(client_config_->group_name());
      client_instance_->shutdown();

      service_state_ = SHUTDOWN_ALREADY;
      break;
    }
    case SHUTDOWN_ALREADY:
    case CREATE_JUST:
      break;
    default:
      break;
  }
}

std::vector<MQMessageQueue> DefaultMQProducerImpl::fetchPublishMessageQueues(const std::string& topic) {
  auto topicPublishInfo = client_instance_->tryToFindTopicPublishInfo(topic);
  if (topicPublishInfo != nullptr) {
    return topicPublishInfo->getMessageQueueList();
  } else {
    return std::vector<MQMessageQueue>{};
  }
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg) {
  return send(msg, dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout());
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, long timeout) {
  try {
    std::unique_ptr<SendResult> sendResult(sendDefaultImpl(msg.getMessageImpl(), SYNC, nullptr, timeout));
    return *sendResult;
  } catch (MQException& e) {
    LOG_ERROR_NEW("send failed, exception:{}", e.what());
    throw;
  }
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, const MQMessageQueue& mq) {
  return send(msg, mq, dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout());
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, const MQMessageQueue& mq, long timeout) {
  Validators::checkMessage(msg, dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->max_message_size());

  if (msg.topic() != mq.topic()) {
    THROW_MQEXCEPTION(MQClientException, "message's topic not equal mq's topic", -1);
  }

  try {
    std::unique_ptr<SendResult> sendResult(sendKernelImpl(msg.getMessageImpl(), mq, SYNC, nullptr, nullptr, timeout));
    return *sendResult;
  } catch (MQException& e) {
    LOG_ERROR_NEW("send failed, exception:{}", e.what());
    throw;
  }
}

void DefaultMQProducerImpl::send(MQMessage& msg, SendCallback* sendCallback) noexcept {
  return send(msg, sendCallback, dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout());
}

void DefaultMQProducerImpl::send(MQMessage& msg, SendCallback* sendCallback, long timeout) noexcept {
  auto msg_impl = msg.getMessageImpl();
  async_send_executor_->submit([this, msg_impl, sendCallback, timeout] {
    try {
      (void)sendDefaultImpl(msg_impl, ASYNC, sendCallback, timeout);
    } catch (MQException& e) {
      LOG_ERROR_NEW("send failed, exception:{}", e.what());
      sendCallback->invokeOnException(e);
    } catch (std::exception& e) {
      LOG_FATAL_NEW("[BUG] encounter unexcepted exception: {}", e.what());
      exit(-1);
    }
  });
}

void DefaultMQProducerImpl::send(MQMessage& msg, const MQMessageQueue& mq, SendCallback* sendCallback) noexcept {
  return send(msg, mq, sendCallback, dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout());
}

void DefaultMQProducerImpl::send(MQMessage& msg,
                                 const MQMessageQueue& mq,
                                 SendCallback* sendCallback,
                                 long timeout) noexcept {
  auto msg_impl = msg.getMessageImpl();
  async_send_executor_->submit([this, msg_impl, mq, sendCallback, timeout] {
    try {
      Validators::checkMessage(*msg_impl,
                               dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->max_message_size());

      if (msg_impl->topic() != mq.topic()) {
        THROW_MQEXCEPTION(MQClientException, "message's topic not equal mq's topic", -1);
      }

      try {
        sendKernelImpl(msg_impl, mq, ASYNC, sendCallback, nullptr, timeout);
      } catch (MQBrokerException& e) {
        std::string info = std::string("unknown exception, ") + e.what();
        THROW_MQEXCEPTION(MQClientException, info, e.GetError());
      }
    } catch (MQException& e) {
      LOG_ERROR_NEW("send failed, exception:{}", e.what());
      sendCallback->invokeOnException(e);
    } catch (std::exception& e) {
      LOG_FATAL_NEW("[BUG] encounter unexcepted exception: {}", e.what());
      exit(-1);
    }
  });
}

void DefaultMQProducerImpl::sendOneway(MQMessage& msg) {
  try {
    sendDefaultImpl(msg.getMessageImpl(), ONEWAY, nullptr,
                    dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout());
  } catch (MQBrokerException& e) {
    std::string info = std::string("unknown exception, ") + e.what();
    THROW_MQEXCEPTION(MQClientException, info, e.GetError());
  }
}

void DefaultMQProducerImpl::sendOneway(MQMessage& msg, const MQMessageQueue& mq) {
  Validators::checkMessage(msg, dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->max_message_size());

  if (msg.topic() != mq.topic()) {
    THROW_MQEXCEPTION(MQClientException, "message's topic not equal mq's topic", -1);
  }

  try {
    sendKernelImpl(msg.getMessageImpl(), mq, ONEWAY, nullptr, nullptr,
                   dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout());
  } catch (MQBrokerException& e) {
    std::string info = std::string("unknown exception, ") + e.what();
    THROW_MQEXCEPTION(MQClientException, info, e.GetError());
  }
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, MessageQueueSelector* selector, void* arg) {
  return send(msg, selector, arg, dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout());
}

SendResult DefaultMQProducerImpl::send(MQMessage& msg, MessageQueueSelector* selector, void* arg, long timeout) {
  try {
    std::unique_ptr<SendResult> result(sendSelectImpl(msg.getMessageImpl(), selector, arg, SYNC, nullptr, timeout));
    return *result.get();
  } catch (MQException& e) {
    LOG_ERROR_NEW("send failed, exception:{}", e.what());
    throw;
  }
}

void DefaultMQProducerImpl::send(MQMessage& msg,
                                 MessageQueueSelector* selector,
                                 void* arg,
                                 SendCallback* sendCallback) noexcept {
  return send(msg, selector, arg, sendCallback,
              dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout());
}

void DefaultMQProducerImpl::send(MQMessage& msg,
                                 MessageQueueSelector* selector,
                                 void* arg,
                                 SendCallback* sendCallback,
                                 long timeout) noexcept {
  auto msg_impl = msg.getMessageImpl();
  async_send_executor_->submit([this, msg_impl, selector, arg, sendCallback, timeout] {
    try {
      try {
        sendSelectImpl(msg_impl, selector, arg, ASYNC, sendCallback, timeout);
      } catch (MQBrokerException& e) {
        std::string info = std::string("unknown exception, ") + e.what();
        THROW_MQEXCEPTION(MQClientException, info, e.GetError());
      }
    } catch (MQException& e) {
      LOG_ERROR_NEW("send failed, exception:{}", e.what());
      sendCallback->invokeOnException(e);
    } catch (std::exception& e) {
      LOG_FATAL_NEW("[BUG] encounter unexcepted exception: {}", e.what());
      exit(-1);
    }
  });
}

void DefaultMQProducerImpl::sendOneway(MQMessage& msg, MessageQueueSelector* selector, void* arg) {
  try {
    sendSelectImpl(msg.getMessageImpl(), selector, arg, ONEWAY, nullptr,
                   dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout());
  } catch (MQBrokerException& e) {
    std::string info = std::string("unknown exception, ") + e.what();
    THROW_MQEXCEPTION(MQClientException, info, e.GetError());
  }
}

TransactionSendResult DefaultMQProducerImpl::sendMessageInTransaction(MQMessage& msg, void* arg) {
  try {
    std::unique_ptr<TransactionSendResult> sendResult(sendMessageInTransactionImpl(
        msg.getMessageImpl(), arg, dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->send_msg_timeout()));
    return *sendResult;
  } catch (MQException& e) {
    LOG_ERROR_NEW("sendMessageInTransaction failed, exception:{}", e.what());
    throw;
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

void DefaultMQProducerImpl::send(std::vector<MQMessage>& msgs, SendCallback* sendCallback) {
  MQMessage batchMessage(batch(msgs));
  send(batchMessage, sendCallback);
}

void DefaultMQProducerImpl::send(std::vector<MQMessage>& msgs, SendCallback* sendCallback, long timeout) {
  MQMessage batchMessage(batch(msgs));
  send(batchMessage, sendCallback, timeout);
}

void DefaultMQProducerImpl::send(std::vector<MQMessage>& msgs, const MQMessageQueue& mq, SendCallback* sendCallback) {
  MQMessage batchMessage(batch(msgs));
  send(batchMessage, mq, sendCallback);
}

void DefaultMQProducerImpl::send(std::vector<MQMessage>& msgs,
                                 const MQMessageQueue& mq,
                                 SendCallback* sendCallback,
                                 long timeout) {
  MQMessage batchMessage(batch(msgs));
  send(batchMessage, mq, sendCallback, timeout);
}

MessagePtr DefaultMQProducerImpl::batch(std::vector<MQMessage>& msgs) {
  if (msgs.size() < 1) {
    THROW_MQEXCEPTION(MQClientException, "msgs need one message at least", -1);
  }

  try {
    auto messageBatch = MessageBatch::generateFromList(msgs);
    for (auto& message : messageBatch->messages()) {
      Validators::checkMessage(message,
                               dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->max_message_size());
      MessageClientIDSetter::setUniqID(const_cast<MQMessage&>(message));
    }
    messageBatch->set_body(messageBatch->encode());
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
      if (requestResponseFuture->send_request_ok()) {
        std::string info = "send request message to <" + msg.topic() + "> OK, but wait reply message timeout, " +
                           UtilAll::to_string(timeout) + " ms.";
        THROW_MQEXCEPTION(RequestTimeoutException, info, ClientErrorCode::REQUEST_TIMEOUT_EXCEPTION);
      } else {
        std::string info = "send request message to <" + msg.topic() + "> fail";
        THROW_MQEXCEPTION2(MQClientException, info, -1, requestResponseFuture->cause());
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
      if (requestResponseFuture->send_request_ok()) {
        std::string info = "send request message to <" + msg.topic() + "> OK, but wait reply message timeout, " +
                           UtilAll::to_string(timeout) + " ms.";
        THROW_MQEXCEPTION(RequestTimeoutException, info, ClientErrorCode::REQUEST_TIMEOUT_EXCEPTION);
      } else {
        std::string info = "send request message to <" + msg.topic() + "> fail";
        THROW_MQEXCEPTION2(MQClientException, info, -1, requestResponseFuture->cause());
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
      if (requestResponseFuture->send_request_ok()) {
        std::string info = "send request message to <" + msg.topic() + "> OK, but wait reply message timeout, " +
                           UtilAll::to_string(timeout) + " ms.";
        THROW_MQEXCEPTION(RequestTimeoutException, info, ClientErrorCode::REQUEST_TIMEOUT_EXCEPTION);
      } else {
        std::string info = "send request message to <" + msg.topic() + "> fail";
        THROW_MQEXCEPTION2(MQClientException, info, -1, requestResponseFuture->cause());
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
  const auto& requestClientId = client_instance_->getClientId();
  MessageAccessor::putProperty(msg, MQMessageConst::PROPERTY_CORRELATION_ID, correlationId);
  MessageAccessor::putProperty(msg, MQMessageConst::PROPERTY_MESSAGE_REPLY_TO_CLIENT, requestClientId);
  MessageAccessor::putProperty(msg, MQMessageConst::PROPERTY_MESSAGE_TTL, UtilAll::to_string(timeout));

  auto hasRouteData = client_instance_->getTopicRouteData(msg.topic()) != nullptr;
  if (!hasRouteData) {
    auto beginTimestamp = UtilAll::currentTimeMillis();
    client_instance_->tryToFindTopicPublishInfo(msg.topic());
    client_instance_->sendHeartbeatToAllBrokerWithLock();
    auto cost = UtilAll::currentTimeMillis() - beginTimestamp;
    if (cost > 500) {
      LOG_WARN_NEW("prepare send request for <{}> cost {} ms", msg.topic(), cost);
    }
  }
}

SendResult* DefaultMQProducerImpl::sendDefaultImpl(MessagePtr msg,
                                                   CommunicationMode communicationMode,
                                                   SendCallback* sendCallback,
                                                   long timeout) {
  Validators::checkMessage(*msg, dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->max_message_size());

  uint64_t beginTimestampFirst = UtilAll::currentTimeMillis();
  uint64_t beginTimestampPrev = beginTimestampFirst;
  uint64_t endTimestamp = beginTimestampFirst;
  auto topicPublishInfo = client_instance_->tryToFindTopicPublishInfo(msg->topic());
  if (topicPublishInfo != nullptr && topicPublishInfo->ok()) {
    bool callTimeout = false;
    std::unique_ptr<SendResult> sendResult;
    int timesTotal = communicationMode == CommunicationMode::SYNC
                         ? 1 + dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->retry_times()
                         : 1;
    int times = 0;
    std::string lastBrokerName;
    for (; times < timesTotal; times++) {
      const auto& mq = selectOneMessageQueue(topicPublishInfo.get(), lastBrokerName);
      lastBrokerName = mq.broker_name();

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
        updateFaultItem(mq.broker_name(), endTimestamp - beginTimestampPrev, false);
        switch (communicationMode) {
          case ASYNC:
            return nullptr;
          case ONEWAY:
            return nullptr;
          case SYNC:
            if (sendResult->send_status() != SEND_OK) {
              if (dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())
                      ->retry_another_broker_when_not_store_ok()) {
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
        updateFaultItem(mq.broker_name(), endTimestamp - beginTimestampPrev, true);
        LOG_WARN_NEW("send failed of times:{}, brokerName:{}. exception:{}", times, mq.broker_name(), e.what());
        continue;
      }

    }  // end of for

    if (sendResult != nullptr) {
      return sendResult.release();
    }

    std::string info = "Send [" + UtilAll::to_string(times) + "] times, still failed, cost [" +
                       UtilAll::to_string(UtilAll::currentTimeMillis() - beginTimestampFirst) +
                       "]ms, Topic: " + msg->topic();
    THROW_MQEXCEPTION(MQClientException, info, -1);
  }

  THROW_MQEXCEPTION(MQClientException, "No route info of this topic: " + msg->topic(), -1);
}

const MQMessageQueue& DefaultMQProducerImpl::selectOneMessageQueue(const TopicPublishInfo* tpInfo,
                                                                   const std::string& lastBrokerName) {
  return mq_fault_strategy_->selectOneMessageQueue(tpInfo, lastBrokerName);
}

void DefaultMQProducerImpl::updateFaultItem(const std::string& brokerName, const long currentLatency, bool isolation) {
  mq_fault_strategy_->updateFaultItem(brokerName, currentLatency, isolation);
}

SendResult* DefaultMQProducerImpl::sendKernelImpl(MessagePtr msg,
                                                  const MQMessageQueue& mq,
                                                  CommunicationMode communicationMode,
                                                  SendCallback* sendCallback,
                                                  TopicPublishInfoPtr topicPublishInfo,
                                                  long timeout) {
  uint64_t beginStartTime = UtilAll::currentTimeMillis();
  std::string brokerAddr = client_instance_->findBrokerAddressInPublish(mq.broker_name());
  if (brokerAddr.empty()) {
    client_instance_->tryToFindTopicPublishInfo(mq.topic());
    brokerAddr = client_instance_->findBrokerAddressInPublish(mq.broker_name());
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
      requestHeader->producerGroup = client_config_->group_name();
      requestHeader->topic = msg->topic();
      requestHeader->defaultTopic = AUTO_CREATE_TOPIC_KEY_TOPIC;
      requestHeader->defaultTopicQueueNums = 4;
      requestHeader->queueId = mq.queue_id();
      requestHeader->sysFlag = sysFlag;
      requestHeader->bornTimestamp = UtilAll::currentTimeMillis();
      requestHeader->flag = msg->flag();
      requestHeader->properties = MessageDecoder::messageProperties2String(msg->properties());
      requestHeader->reconsumeTimes = 0;
      requestHeader->unitMode = false;
      requestHeader->batch = msg->isBatch();

      if (UtilAll::isRetryTopic(mq.topic())) {
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
        case ASYNC: {
          long costTimeAsync = UtilAll::currentTimeMillis() - beginStartTime;
          if (timeout < costTimeAsync) {
            THROW_MQEXCEPTION(RemotingTooMuchRequestException, "sendKernelImpl call timeout", -1);
          }
          sendResult = client_instance_->getMQClientAPIImpl()->sendMessage(
              brokerAddr, mq.broker_name(), msg, std::move(requestHeader), timeout, communicationMode, sendCallback,
              topicPublishInfo, client_instance_,
              dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->retry_times_for_async(),
              shared_from_this());
        } break;
        case ONEWAY:
        case SYNC: {
          long costTimeSync = UtilAll::currentTimeMillis() - beginStartTime;
          if (timeout < costTimeSync) {
            THROW_MQEXCEPTION(RemotingTooMuchRequestException, "sendKernelImpl call timeout", -1);
          }
          sendResult = client_instance_->getMQClientAPIImpl()->sendMessage(brokerAddr, mq.broker_name(), msg,
                                                                           std::move(requestHeader), timeout,
                                                                           communicationMode, shared_from_this());
        } break;
        default:
          assert(false);
          break;
      }

      return sendResult;
    } catch (MQException& e) {
      throw;
    }
  }

  THROW_MQEXCEPTION(MQClientException, "The broker[" + mq.broker_name() + "] not exist", -1);
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

  const auto& body = msg.body();
  if (body.size() >= dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->compress_msg_body_over_howmuch()) {
    std::string out_body;
    if (UtilAll::deflate(body, out_body,
                         dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->compress_level())) {
      msg.set_body(std::move(out_body));
      msg.putProperty(MQMessageConst::PROPERTY_ALREADY_COMPRESSED_FLAG, "true");
      return true;
    }
  }

  return false;
}

SendResult* DefaultMQProducerImpl::sendSelectImpl(MessagePtr msg,
                                                  MessageQueueSelector* selector,
                                                  void* arg,
                                                  CommunicationMode communicationMode,
                                                  SendCallback* sendCallback,
                                                  long timeout) {
  auto beginStartTime = UtilAll::currentTimeMillis();
  Validators::checkMessage(*msg, dynamic_cast<DefaultMQProducerConfig*>(client_config_.get())->max_message_size());

  TopicPublishInfoPtr topicPublishInfo = client_instance_->tryToFindTopicPublishInfo(msg->topic());
  if (topicPublishInfo != nullptr && topicPublishInfo->ok()) {
    MQMessageQueue mq = selector->select(topicPublishInfo->getMessageQueueList(), MQMessage(msg), arg);

    auto costTime = UtilAll::currentTimeMillis() - beginStartTime;
    if (timeout < costTime) {
      THROW_MQEXCEPTION(RemotingTooMuchRequestException, "sendSelectImpl call timeout", -1);
    }

    return sendKernelImpl(msg, mq, communicationMode, sendCallback, nullptr, timeout - costTime);
  }

  std::string info = std::string("No route info for this topic, ") + msg->topic();
  THROW_MQEXCEPTION(MQClientException, info, -1);
}

void DefaultMQProducerImpl::initTransactionEnv() {
  if (nullptr == check_transaction_executor_) {
    check_transaction_executor_.reset(new thread_pool_executor(1, false));
  }
  check_transaction_executor_->startup();
}

void DefaultMQProducerImpl::destroyTransactionEnv() {
  check_transaction_executor_->shutdown();
}

TransactionListener* DefaultMQProducerImpl::getCheckListener() {
  auto transactionProducerConfig = dynamic_cast<TransactionMQProducerConfig*>(client_config_.get());
  if (transactionProducerConfig != nullptr) {
    return transactionProducerConfig->getTransactionListener();
  }
  return nullptr;
};

TransactionSendResult* DefaultMQProducerImpl::sendMessageInTransactionImpl(MessagePtr msg, void* arg, long timeout) {
  auto* transactionListener = getCheckListener();
  if (nullptr == transactionListener) {
    THROW_MQEXCEPTION(MQClientException, "transactionListener is null", -1);
  }

  std::unique_ptr<SendResult> sendResult;
  MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_TRANSACTION_PREPARED, "true");
  MessageAccessor::putProperty(*msg, MQMessageConst::PROPERTY_PRODUCER_GROUP, client_config_->group_name());
  try {
    sendResult.reset(sendDefaultImpl(msg, SYNC, nullptr, timeout));
  } catch (MQException& e) {
    THROW_MQEXCEPTION(MQClientException, "send message Exception", -1);
  }

  LocalTransactionState localTransactionState = LocalTransactionState::UNKNOWN;
  std::exception_ptr localException;
  switch (sendResult->send_status()) {
    case SendStatus::SEND_OK:
      try {
        if (!sendResult->transaction_id().empty()) {
          msg->putProperty("__transactionId__", sendResult->transaction_id());
        }
        const auto& transactionId = msg->getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
        if (!transactionId.empty()) {
          msg->set_transaction_id(transactionId);
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
  transactionSendResult->set_transaction_id(msg->transaction_id());
  transactionSendResult->set_local_transaction_state(localTransactionState);
  return transactionSendResult;
}

void DefaultMQProducerImpl::endTransaction(SendResult& sendResult,
                                           LocalTransactionState localTransactionState,
                                           std::exception_ptr& localException) {
  const auto& msg_id = !sendResult.offset_msg_id().empty() ? sendResult.offset_msg_id() : sendResult.msg_id();
  auto id = MessageDecoder::decodeMessageId(msg_id);
  const auto& transactionId = sendResult.transaction_id();
  std::string brokerAddr = client_instance_->findBrokerAddressInPublish(sendResult.message_queue().broker_name());
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

  requestHeader->producerGroup = client_config_->group_name();
  requestHeader->tranStateTableOffset = sendResult.queue_offset();
  requestHeader->msgId = sendResult.msg_id();

  std::string remark =
      localException ? ("executeLocalTransactionBranch exception: " + UtilAll::to_string(localException)) : null;

  client_instance_->getMQClientAPIImpl()->endTransactionOneway(brokerAddr, requestHeader, remark);
}

void DefaultMQProducerImpl::checkTransactionState(const std::string& addr,
                                                  MessageExtPtr msg,
                                                  CheckTransactionStateRequestHeader* checkRequestHeader) {
  long tranStateTableOffset = checkRequestHeader->tranStateTableOffset;
  long commitLogOffset = checkRequestHeader->commitLogOffset;
  std::string msgId = checkRequestHeader->msgId;
  std::string transactionId = checkRequestHeader->transactionId;
  std::string offsetMsgId = checkRequestHeader->offsetMsgId;

  check_transaction_executor_->submit(std::bind(&DefaultMQProducerImpl::checkTransactionStateImpl, this, addr, msg,
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
                 client_config_->group_name());
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
  endHeader->producerGroup = client_config_->group_name();
  endHeader->tranStateTableOffset = tranStateTableOffset;
  endHeader->fromTransactionCheck = true;

  std::string uniqueKey = message->getProperty(MQMessageConst::PROPERTY_UNIQ_CLIENT_MESSAGE_ID_KEYIDX);
  if (uniqueKey.empty()) {
    uniqueKey = message->msg_id();
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
    client_instance_->getMQClientAPIImpl()->endTransactionOneway(addr, endHeader, remark);
  } catch (std::exception& e) {
    LOG_ERROR_NEW("endTransactionOneway exception: {}", e.what());
  }
}

bool DefaultMQProducerImpl::isSendLatencyFaultEnable() const {
  return mq_fault_strategy_->isSendLatencyFaultEnable();
}

void DefaultMQProducerImpl::setSendLatencyFaultEnable(bool sendLatencyFaultEnable) {
  mq_fault_strategy_->setSendLatencyFaultEnable(sendLatencyFaultEnable);
}

}  // namespace rocketmq
