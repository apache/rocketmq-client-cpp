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

#include "AsyncCallbackWrap.h"

#include "Logging.h"
#include "MQClientAPIImpl.h"
#include "MQDecoder.h"
#include "MQMessageQueue.h"
#include "MQProtos.h"
#include "PullAPIWrapper.h"
#include "PullResultExt.h"

namespace rocketmq {

//######################################
// SendCallbackWrap
//######################################

SendCallbackWrap::SendCallbackWrap(const string& addr,
                                   const string& brokerName,
                                   const MQMessage& msg,
                                   const RemotingCommand& requestCommand,
                                   int maxRetrySendTimes,
                                   int retrySendTimes,
                                   SendCallback* pSendCallback,
                                   MQClientAPIImpl* pClientAPI,
                                   SendMessageDelegate retrySendDelegate)
    : m_pSendCallback(pSendCallback),
      m_pClientAPI(pClientAPI),
      m_addr(addr),
      m_brokerName(brokerName),
      m_msg(msg),
      m_requestCommand(requestCommand),
      m_retrySendTimes(retrySendTimes),
      m_maxRetrySendTimes(maxRetrySendTimes),
      m_retrySendDelegate(retrySendDelegate) {}

void SendCallbackWrap::operationComplete(ResponseFuture* responseFuture) noexcept {
  if (responseFuture != nullptr) {
    std::unique_ptr<RemotingCommand> response(responseFuture->getResponseCommand());  // avoid RemotingCommand leak
    if (m_pSendCallback == nullptr) {
      return;
    }

    if (response) {
      int opaque = responseFuture->getOpaque();
      try {
        SendResult sendResult = m_pClientAPI->processSendResponse(m_brokerName, m_msg, response.get());
        LOG_DEBUG("operationComplete: processSendResponse success, opaque:%d, maxRetryTime:%d, retrySendTimes:%d",
                  opaque, m_maxRetrySendTimes, m_retrySendTimes);
        try {
          m_pSendCallback->onSuccess(sendResult);
        } catch (...) {
        }
      } catch (MQException& e) {
        LOG_ERROR("operationComplete: processSendResponse exception: %s", e.what());
        // broker may return exception, need consider retry send
        onExceptionImpl(responseFuture);
      }
    } else {
      std::string err;
      if (!responseFuture->isSendRequestOK()) {
        err = "send request failed";
      } else if (responseFuture->isTimeout()) {
        err = "wait response timeout";
      } else {
        err = "unknown reason";
      }
      MQException exception(err, -1, __FILE__, __LINE__);
      m_pSendCallback->onException(exception);
    }
  } else {
    if (m_pSendCallback == nullptr) {
      return;
    }

    std::string err = "send request failed";
    MQException exception(err, -1, __FILE__, __LINE__);
    m_pSendCallback->onException(exception);
  }

  if (m_pSendCallback->getSendCallbackType() == SEND_CALLBACK_TYPE_ATUO_DELETE) {
    deleteAndZero(m_pSendCallback);
    m_pSendCallback = nullptr;
  }
}

void SendCallbackWrap::onExceptionImpl(ResponseFuture* responseFuture) {
  if (m_retrySendTimes < m_maxRetrySendTimes && m_maxRetrySendTimes > 1) {
    int64 left_timeout_ms = responseFuture->leftTime();
    m_retrySendTimes += 1;
    LOG_WARN("retry send, opaque:%d, sendTimes:%d, maxRetryTimes:%d, left_timeout:%lld, brokerAddr:%s, msg:%s",
             responseFuture->getOpaque(), m_retrySendTimes, m_maxRetrySendTimes, left_timeout_ms, m_addr.data(),
             m_msg.toString().data());

    try {
      m_retrySendDelegate(this, left_timeout_ms);
      return;  // send retry again, here need return
    } catch (MQClientException& e) {
      LOG_ERROR("retry send exception:%s, opaque:%d, retryTimes:%d, msg:%s, not retry send again", e.what(),
                responseFuture->getOpaque(), m_retrySendTimes, m_msg.toString().data());
    }
  }

  MQException exception("process send response error", -1, __FILE__, __LINE__);
  m_pSendCallback->onException(exception);
}

//######################################
// PullCallbackWrap
//######################################

PullCallbackWrap::PullCallbackWrap(PullCallback* pPullCallback, MQClientAPIImpl* pClientAPI, void* pArg)
    : m_pPullCallback(pPullCallback), m_pClientAPI(pClientAPI) {
  m_pArg = *static_cast<AsyncArg*>(pArg);
}

void PullCallbackWrap::operationComplete(ResponseFuture* responseFuture) noexcept {
  if (responseFuture != nullptr) {
    std::unique_ptr<RemotingCommand> response(responseFuture->getResponseCommand());  // avoid RemotingCommand leak

    if (m_pPullCallback == nullptr) {
      LOG_ERROR("m_pPullCallback is NULL, AsyncPull could not continue");
      return;
    }

    if (response) {
      try {
        if (m_pArg.pPullWrapper) {
          std::unique_ptr<PullResult> pullResult(m_pClientAPI->processPullResponse(response.get()));
          PullResult result = m_pArg.pPullWrapper->processPullResult(m_pArg.mq, pullResult.get(), &m_pArg.subData);
          m_pPullCallback->onSuccess(m_pArg.mq, result, true);
        } else {
          LOG_ERROR("pPullWrapper had been destroyed with consumer");
        }
      } catch (MQException& e) {
        LOG_ERROR(e.what());
        MQException exception("pullResult error", -1, __FILE__, __LINE__);
        m_pPullCallback->onException(exception);
      }
    } else {
      std::string err;
      if (!responseFuture->isSendRequestOK()) {
        err = "send request failed";
      } else if (responseFuture->isTimeout()) {
        err = "wait response timeout";
      } else {
        err = "unknown reason";
      }
      MQException exception(err, -1, __FILE__, __LINE__);
      m_pPullCallback->onException(exception);
    }
  } else {
    if (m_pPullCallback == nullptr) {
      LOG_ERROR("m_pPullCallback is NULL, AsyncPull could not continue");
      return;
    }

    std::string err = "send request failed";
    MQException exception(err, -1, __FILE__, __LINE__);
    m_pPullCallback->onException(exception);
  }

  if (m_pPullCallback->getPullCallbackType() == PULL_CALLBACK_TYPE_AUTO_DELETE) {
    deleteAndZero(m_pPullCallback);
    m_pPullCallback = nullptr;
  }
}

}  // namespace rocketmq
