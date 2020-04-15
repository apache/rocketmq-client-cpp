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
#include "SendCallbackWrap.h"

#include <typeindex>

#include "CommandHeader.h"
#include "DefaultMQProducer.h"
#include "Logging.h"
#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "MQDecoder.h"
#include "MQMessageQueue.h"
#include "MQProtos.h"
#include "PullAPIWrapper.h"
#include "PullResultExt.h"
#include "TopicPublishInfo.h"

namespace rocketmq {

SendCallbackWrap::SendCallbackWrap(const std::string& addr,
                                   const std::string& brokerName,
                                   const MQMessagePtr msg,
                                   RemotingCommand&& request,
                                   SendCallback* sendCallback,
                                   TopicPublishInfoPtr topicPublishInfo,
                                   MQClientInstancePtr instance,
                                   int retryTimesWhenSendFailed,
                                   int times,
                                   DefaultMQProducerImplPtr producer)
    : m_addr(addr),
      m_brokerName(brokerName),
      m_msg(msg),
      m_request(std::forward<RemotingCommand>(request)),
      m_sendCallback(sendCallback),
      m_topicPublishInfo(topicPublishInfo),
      m_instance(instance),
      m_timesTotal(retryTimesWhenSendFailed),
      m_times(times),
      m_producer(producer) {}

void SendCallbackWrap::operationComplete(ResponseFuture* responseFuture) noexcept {
  auto producer = m_producer.lock();
  if (nullptr == producer) {
    MQException exception("DefaultMQProducer is released.", -1, __FILE__, __LINE__);
    m_sendCallback->onException(exception);

    // auto delete callback
    if (m_sendCallback->getSendCallbackType() == SEND_CALLBACK_TYPE_AUTO_DELETE) {
      deleteAndZero(m_sendCallback);
    }
    return;
  }

  std::unique_ptr<RemotingCommand> response(responseFuture->getResponseCommand());  // avoid RemotingCommand leak
  if (nullptr == m_sendCallback && response != nullptr) {
    // TODO: executeSendMessageHookAfter
    // try {
    //   std::unique_ptr<SendResult> sendResult(m_pClientAPI->processSendResponse(m_brokerName, m_msg, response.get()));
    //   if (context != null && sendResult != nullptr) {
    //     context.setSendResult(sendResult);
    //     context.getProducer().executeSendMessageHookAfter(context);
    //   }
    // } catch (...) {
    // }

    producer->updateFaultItem(m_brokerName, UtilAll::currentTimeMillis() - responseFuture->getBeginTimestamp(), false);
    return;
  }

  if (response != nullptr) {
    int opaque = responseFuture->getOpaque();
    try {
      std::unique_ptr<SendResult> sendResult(
          m_instance->getMQClientAPIImpl()->processSendResponse(m_brokerName, m_msg, response.get()));
      assert(sendResult != nullptr);

      LOG_DEBUG("operationComplete: processSendResponse success, opaque:%d, maxRetryTime:%d, retrySendTimes:%d", opaque,
                m_timesTotal, m_times);

      // TODO: executeSendMessageHookAfter
      // if (context != null) {
      //   context.setSendResult(sendResult);
      //   context.getProducer().executeSendMessageHookAfter(context);
      // }

      try {
        m_sendCallback->onSuccess(*sendResult);
      } catch (...) {
      }

      producer->updateFaultItem(m_brokerName, UtilAll::currentTimeMillis() - responseFuture->getBeginTimestamp(),
                                false);

      // auto delete callback
      if (m_sendCallback->getSendCallbackType() == SEND_CALLBACK_TYPE_AUTO_DELETE) {
        deleteAndZero(m_sendCallback);
      }
    } catch (MQException& e) {
      producer->updateFaultItem(m_brokerName, UtilAll::currentTimeMillis() - responseFuture->getBeginTimestamp(), true);
      LOG_ERROR("operationComplete: processSendResponse exception: %s", e.what());
      return onExceptionImpl(responseFuture, responseFuture->leftTime(), e, false);
    }
  } else {
    producer->updateFaultItem(m_brokerName, UtilAll::currentTimeMillis() - responseFuture->getBeginTimestamp(), true);
    std::string err;
    if (!responseFuture->isSendRequestOK()) {
      err = "send request failed";
    } else if (responseFuture->isTimeout()) {
      err = "wait response timeout";
    } else {
      err = "unknown reason";
    }
    MQException exception(err, -1, __FILE__, __LINE__);
    return onExceptionImpl(responseFuture, responseFuture->leftTime(), exception, true);
  }
}

void SendCallbackWrap::onExceptionImpl(ResponseFuture* responseFuture,
                                       long timeoutMillis,
                                       MQException& e,
                                       bool needRetry) {
  auto producer = m_producer.lock();
  if (nullptr == producer) {
    MQException exception("DefaultMQProducer is released.", -1, __FILE__, __LINE__);
    m_sendCallback->onException(exception);

    // auto delete callback
    if (m_sendCallback->getSendCallbackType() == SEND_CALLBACK_TYPE_AUTO_DELETE) {
      deleteAndZero(m_sendCallback);
    }
    return;
  }

  m_times++;
  if (needRetry && m_times <= m_timesTotal) {
    std::string retryBrokerName = m_brokerName;  // by default, it will send to the same broker
    if (m_topicPublishInfo != nullptr) {
      // select one message queue accordingly, in order to determine which broker to send
      const auto& mqChosen = producer->selectOneMessageQueue(m_topicPublishInfo.get(), m_brokerName);
      retryBrokerName = mqChosen.getBrokerName();

      // set queueId to requestHeader
      auto* requestHeader = m_request.readCustomHeader();
      if (std::type_index(typeid(*requestHeader)) == std::type_index(typeid(SendMessageRequestHeaderV2))) {
        static_cast<SendMessageRequestHeaderV2*>(requestHeader)->e = mqChosen.getQueueId();
      } else {
        static_cast<SendMessageRequestHeader*>(requestHeader)->queueId = mqChosen.getQueueId();
      }
    }
    std::string addr = m_instance->findBrokerAddressInPublish(retryBrokerName);
    LOG_INFO_NEW("async send msg by retry {} times. topic={}, brokerAddr={}, brokerName={}", m_times, m_msg->getTopic(),
                 addr, retryBrokerName);
    try {
      // new request
      m_request.setOpaque(RemotingCommand::createNewRequestId());

      // resend
      m_addr = std::move(addr);
      m_brokerName = std::move(retryBrokerName);
      m_instance->getMQClientAPIImpl()->sendMessageAsyncImpl(this, timeoutMillis);

      responseFuture->releaseInvokeCallback();  // for avoid delete this SendCallbackWrap
      return;
    } catch (MQException& e1) {
      producer->updateFaultItem(m_brokerName, 3000, true);
      return onExceptionImpl(responseFuture, responseFuture->leftTime(), e1, true);
    }
  } else {
    m_sendCallback->onException(e);

    // auto delete callback
    if (m_sendCallback->getSendCallbackType() == SEND_CALLBACK_TYPE_AUTO_DELETE) {
      deleteAndZero(m_sendCallback);
    }
  }
}

}  // namespace rocketmq
