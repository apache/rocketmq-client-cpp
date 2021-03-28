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

#include <cassert>

#include <typeindex>

#include "DefaultMQProducer.h"
#include "Logging.h"
#include "MQClientAPIImpl.h"
#include "MQClientInstance.h"
#include "MQMessageQueue.h"
#include "MQProtos.h"
#include "MessageDecoder.h"
#include "PullAPIWrapper.h"
#include "PullResultExt.hpp"
#include "TopicPublishInfo.hpp"
#include "protocol/header/CommandHeader.h"

namespace rocketmq {

void SendCallback::invokeOnSuccess(SendResult& send_result) noexcept {
  auto type = getSendCallbackType();
  try {
    onSuccess(send_result);
  } catch (const std::exception& e) {
    LOG_WARN_NEW("encounter exception when invoke SendCallback::onSuccess(), {}", e.what());
  }
  if (type == SendCallbackType::kAutoDelete) {
    delete this;
  }
}

void SendCallback::invokeOnException(MQException& exception) noexcept {
  auto type = getSendCallbackType();
  try {
    onException(exception);
  } catch (const std::exception& e) {
    LOG_WARN_NEW("encounter exception when invoke SendCallback::onException(), {}", e.what());
  }
  if (type == SendCallbackType::kAutoDelete) {
    delete this;
  }
}

SendCallbackWrap::SendCallbackWrap(const std::string& addr,
                                   const std::string& brokerName,
                                   const MessagePtr msg,
                                   RemotingCommand&& request,
                                   SendCallback* sendCallback,
                                   TopicPublishInfoPtr topicPublishInfo,
                                   MQClientInstancePtr instance,
                                   int retryTimesWhenSendFailed,
                                   int times,
                                   DefaultMQProducerImplPtr producer)
    : addr_(addr),
      broker_name_(brokerName),
      msg_(msg),
      request_(std::forward<RemotingCommand>(request)),
      send_callback_(sendCallback),
      topic_publish_info_(topicPublishInfo),
      instance_(instance),
      times_total_(retryTimesWhenSendFailed),
      times_(times),
      producer_(producer) {}

void SendCallbackWrap::operationComplete(ResponseFuture* responseFuture) noexcept {
  auto producer = producer_.lock();
  if (nullptr == producer) {
    MQException exception("DefaultMQProducer is released.", -1, __FILE__, __LINE__);
    send_callback_->invokeOnException(exception);
    return;
  }

  std::unique_ptr<RemotingCommand> response(responseFuture->getResponseCommand());  // avoid RemotingCommand leak
  if (nullptr == send_callback_ && response != nullptr) {
    // TODO: executeSendMessageHookAfter
    // try {
    //   std::unique_ptr<SendResult> sendResult(m_pClientAPI->processSendResponse(m_brokerName, m_msg, response.get()));
    //   if (context != null && sendResult != nullptr) {
    //     context.setSendResult(sendResult);
    //     context.getProducer().executeSendMessageHookAfter(context);
    //   }
    // } catch (...) {
    // }

    producer->updateFaultItem(broker_name_, UtilAll::currentTimeMillis() - responseFuture->begin_timestamp(), false);
    return;
  }

  if (response != nullptr) {
    int opaque = responseFuture->opaque();
    try {
      std::unique_ptr<SendResult> sendResult(
          instance_->getMQClientAPIImpl()->processSendResponse(broker_name_, msg_, response.get()));
      assert(sendResult != nullptr);

      LOG_DEBUG("operationComplete: processSendResponse success, opaque:%d, maxRetryTime:%d, retrySendTimes:%d", opaque,
                times_total_, times_);

      // TODO: executeSendMessageHookAfter
      // if (context != null) {
      //   context.setSendResult(sendResult);
      //   context.getProducer().executeSendMessageHookAfter(context);
      // }

      try {
        send_callback_->invokeOnSuccess(*sendResult);
      } catch (...) {
      }

      producer->updateFaultItem(broker_name_, UtilAll::currentTimeMillis() - responseFuture->begin_timestamp(), false);
    } catch (MQException& e) {
      producer->updateFaultItem(broker_name_, UtilAll::currentTimeMillis() - responseFuture->begin_timestamp(), true);
      LOG_ERROR("operationComplete: processSendResponse exception: %s", e.what());
      return onExceptionImpl(responseFuture, responseFuture->leftTime(), e, false);
    }
  } else {
    producer->updateFaultItem(broker_name_, UtilAll::currentTimeMillis() - responseFuture->begin_timestamp(), true);
    std::string err;
    if (!responseFuture->send_request_ok()) {
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
  auto producer = producer_.lock();
  if (nullptr == producer) {
    MQException exception("DefaultMQProducer is released.", -1, __FILE__, __LINE__);
    send_callback_->invokeOnException(exception);
    return;
  }

  times_++;
  if (needRetry && times_ <= times_total_) {
    std::string retryBrokerName = broker_name_;  // by default, it will send to the same broker
    if (topic_publish_info_ != nullptr) {
      // select one message queue accordingly, in order to determine which broker to send
      const auto& mqChosen = producer->selectOneMessageQueue(topic_publish_info_.get(), broker_name_);
      retryBrokerName = mqChosen.broker_name();

      // set queueId to requestHeader
      auto* requestHeader = request_.readCustomHeader();
      if (std::type_index(typeid(*requestHeader)) == std::type_index(typeid(SendMessageRequestHeaderV2))) {
        static_cast<SendMessageRequestHeaderV2*>(requestHeader)->e = mqChosen.queue_id();
      } else {
        static_cast<SendMessageRequestHeader*>(requestHeader)->queueId = mqChosen.queue_id();
      }
    }
    std::string addr = instance_->findBrokerAddressInPublish(retryBrokerName);
    LOG_INFO_NEW("async send msg by retry {} times. topic={}, brokerAddr={}, brokerName={}", times_, msg_->topic(),
                 addr, retryBrokerName);
    try {
      // new request
      request_.set_opaque(RemotingCommand::createNewRequestId());

      // resend
      addr_ = std::move(addr);
      broker_name_ = std::move(retryBrokerName);
      instance_->getMQClientAPIImpl()->sendMessageAsyncImpl(responseFuture->invoke_callback(), timeoutMillis);
      return;
    } catch (MQException& e1) {
      producer->updateFaultItem(broker_name_, 3000, true);
      return onExceptionImpl(responseFuture, responseFuture->leftTime(), e1, true);
    }
  } else {
    send_callback_->invokeOnException(e);
  }
}

}  // namespace rocketmq
