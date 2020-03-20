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
#include "PullAPIWrapper.h"
#include "PullResultExt.h"
#include "ResponseFuture.h"

namespace rocketmq {
//<!***************************************************************************
AsyncCallbackWrap::AsyncCallbackWrap(AsyncCallback* pAsyncCallback, MQClientAPIImpl* pclientAPI)
    : m_pAsyncCallBack(pAsyncCallback), m_pClientAPI(pclientAPI) {}

AsyncCallbackWrap::~AsyncCallbackWrap() {
  m_pAsyncCallBack = NULL;
  m_pClientAPI = NULL;
}

//<!************************************************************************
SendCallbackWrap::SendCallbackWrap(const string& brokerName,
                                   const MQMessage& msg,
                                   AsyncCallback* pAsyncCallback,
                                   MQClientAPIImpl* pclientAPI)
    : AsyncCallbackWrap(pAsyncCallback, pclientAPI), m_msg(msg), m_brokerName(brokerName) {}

void SendCallbackWrap::onException() {
  if (m_pAsyncCallBack == NULL)
    return;

  SendCallback* pCallback = static_cast<SendCallback*>(m_pAsyncCallBack);
  if (pCallback) {
    unique_ptr<MQException> exception(
        new MQException("send msg failed due to wait response timeout or network error", -1, __FILE__, __LINE__));
    pCallback->onException(*exception);
    if (pCallback->getSendCallbackType() == autoDeleteSendCallback) {
      deleteAndZero(pCallback);
    }
  }
}

void SendCallbackWrap::operationComplete(ResponseFuture* pResponseFuture, bool bProducePullRequest) {
  unique_ptr<RemotingCommand> pResponse(pResponseFuture->getCommand());

  if (m_pAsyncCallBack == NULL) {
    return;
  }
  int opaque = pResponseFuture->getOpaque();
  SendCallback* pCallback = static_cast<SendCallback*>(m_pAsyncCallBack);

  if (!pResponse) {
    string err = "unknow reseaon";
    if (!pResponseFuture->isSendRequestOK()) {
      err = "send request failed";

    } else if (pResponseFuture->isTimeOut()) {
      // pResponseFuture->setAsyncResponseFlag();
      err = "wait response timeout";
    }
    if (pCallback) {
      MQException exception(err, -1, __FILE__, __LINE__);
      pCallback->onException(exception);
    }
    LOG_ERROR("send failed of:%d", pResponseFuture->getOpaque());
  } else {
    try {
      SendResult ret = m_pClientAPI->processSendResponse(m_brokerName, m_msg, pResponse.get());
      if (pCallback) {
        LOG_DEBUG("operationComplete: processSendResponse success, opaque:%d, maxRetryTime:%d, retrySendTimes:%d",
                  opaque, pResponseFuture->getMaxRetrySendTimes(), pResponseFuture->getRetrySendTimes());
        pCallback->onSuccess(ret);
      }
    } catch (MQException& e) {
      LOG_ERROR("operationComplete: processSendResponse exception: %s", e.what());

      // broker may return exception, need consider retry send
      int maxRetryTimes = pResponseFuture->getMaxRetrySendTimes();
      int retryTimes = pResponseFuture->getRetrySendTimes();
      if (pResponseFuture->getAsyncFlag() && retryTimes < maxRetryTimes && maxRetryTimes > 1) {
        int64 left_timeout_ms = pResponseFuture->leftTime();
        string brokerAddr = pResponseFuture->getBrokerAddr();
        const RemotingCommand& requestCommand = pResponseFuture->getRequestCommand();
        retryTimes += 1;
        LOG_WARN("retry send, opaque:%d, sendTimes:%d, maxRetryTimes:%d, left_timeout:%lld, brokerAddr:%s, msg:%s",
                 opaque, retryTimes, maxRetryTimes, left_timeout_ms, brokerAddr.data(), m_msg.toString().data());

        bool exception_flag = false;
        try {
          m_pClientAPI->sendMessageAsync(pResponseFuture->getBrokerAddr(), m_brokerName, m_msg,
                                         (RemotingCommand&)requestCommand, pCallback, left_timeout_ms, maxRetryTimes,
                                         retryTimes);
        } catch (MQClientException& e) {
          LOG_ERROR("retry send exception:%s, opaque:%d, retryTimes:%d, msg:%s, not retry send again", e.what(), opaque,
                    retryTimes, m_msg.toString().data());
          exception_flag = true;
        }

        if (exception_flag == false) {
          return;  // send retry again, here need return
        }
      }

      if (pCallback) {
        MQException exception("process send response error", -1, __FILE__, __LINE__);
        pCallback->onException(exception);
      }
    }
  }
  if (pCallback && pCallback->getSendCallbackType() == autoDeleteSendCallback) {
    deleteAndZero(pCallback);
  }
}

//<!************************************************************************
PullCallbackWrap::PullCallbackWrap(AsyncCallback* pAsyncCallback, MQClientAPIImpl* pclientAPI, void* pArg)
    : AsyncCallbackWrap(pAsyncCallback, pclientAPI) {
  m_pArg = *static_cast<AsyncArg*>(pArg);
}

PullCallbackWrap::~PullCallbackWrap() {}

void PullCallbackWrap::onException() {
  if (m_pAsyncCallBack == NULL)
    return;

  PullCallback* pCallback = static_cast<PullCallback*>(m_pAsyncCallBack);
  if (pCallback) {
    MQException exception("wait response timeout", -1, __FILE__, __LINE__);
    pCallback->onException(exception);
  } else {
    LOG_ERROR("PullCallback is NULL, AsyncPull could not continue");
  }
}

void PullCallbackWrap::operationComplete(ResponseFuture* pResponseFuture, bool bProducePullRequest) {
  unique_ptr<RemotingCommand> pResponse(pResponseFuture->getCommand());
  if (m_pAsyncCallBack == NULL) {
    LOG_ERROR("m_pAsyncCallBack is NULL, AsyncPull could not continue");
    return;
  }
  PullCallback* pCallback = static_cast<PullCallback*>(m_pAsyncCallBack);
  if (!pResponse) {
    string err = "unknow reseaon";
    if (!pResponseFuture->isSendRequestOK()) {
      err = "send request failed";

    } else if (pResponseFuture->isTimeOut()) {
      // pResponseFuture->setAsyncResponseFlag();
      err = "wait response timeout";
    }
    MQException exception(err, -1, __FILE__, __LINE__);
    LOG_ERROR("Async pull exception of opaque:%d", pResponseFuture->getOpaque());
    if (pCallback && bProducePullRequest)
      pCallback->onException(exception);
  } else {
    try {
      if (m_pArg.pPullWrapper) {
        unique_ptr<PullResult> pullResult(m_pClientAPI->processPullResponse(pResponse.get()));
        PullResult result = m_pArg.pPullWrapper->processPullResult(m_pArg.mq, pullResult.get(), &m_pArg.subData);
        if (pCallback)
          pCallback->onSuccess(m_pArg.mq, result, bProducePullRequest);
      } else {
        LOG_ERROR("pPullWrapper had been destroyed with consumer");
      }
    } catch (MQException& e) {
      LOG_ERROR("%s", e.what());
      MQException exception("pullResult error", -1, __FILE__, __LINE__);
      if (pCallback && bProducePullRequest)
        pCallback->onException(exception);
    }
  }
}

//<!***************************************************************************
}  // namespace rocketmq
