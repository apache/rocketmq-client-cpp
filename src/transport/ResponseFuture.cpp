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
#include "ResponseFuture.h"

#include <chrono>

#include "Logging.h"
#include "TcpRemotingClient.h"

namespace rocketmq {

//<!************************************************************************
ResponseFuture::ResponseFuture(int requestCode,
                               int opaque,
                               TcpRemotingClient* powner,
                               int64 timeout,
                               bool bAsync,
                               std::shared_ptr<AsyncCallbackWrap> pCallback)
    : m_requestCode(requestCode),
      m_opaque(opaque),
      m_timeout(timeout),
      m_bAsync(bAsync),
      m_pCallbackWrap(pCallback),
      m_asyncCallbackStatus(ASYNC_CALLBACK_STATUS_INIT),
      m_haveResponse(false),
      m_sendRequestOK(false),
      m_pResponseCommand(nullptr),
      m_maxRetrySendTimes(1),
      m_retrySendTimes(1) {
  m_brokerAddr = "";
  m_beginTimestamp = UtilAll::currentTimeMillis();
}

ResponseFuture::~ResponseFuture() {}

void ResponseFuture::releaseThreadCondition() {
  m_defaultEvent.notify_all();
}

RemotingCommand* ResponseFuture::waitResponse(int timeoutMillis) {
  std::unique_lock<std::mutex> eventLock(m_defaultEventLock);
  if (!m_haveResponse) {
    if (timeoutMillis <= 0) {
      timeoutMillis = m_timeout;
    }
    if (m_defaultEvent.wait_for(eventLock, std::chrono::milliseconds(timeoutMillis)) == std::cv_status::timeout) {
      LOG_WARN("waitResponse of code:%d with opaque:%d timeout", m_requestCode, m_opaque);
      m_haveResponse = true;
    }
  }
  return m_pResponseCommand;
}

bool ResponseFuture::setResponse(RemotingCommand* pResponseCommand) {
  std::unique_lock<std::mutex> eventLock(m_defaultEventLock);

  if (m_haveResponse) {
    return false;
  }

  m_pResponseCommand = pResponseCommand;
  m_haveResponse = true;

  if (!getAsyncFlag()) {
    m_defaultEvent.notify_all();
  }

  return true;
}

const bool ResponseFuture::getAsyncFlag() {
  return m_bAsync;
}

bool ResponseFuture::isSendRequestOK() const {
  return m_sendRequestOK;
}

void ResponseFuture::setSendRequestOK(bool sendRequestOK) {
  m_sendRequestOK = sendRequestOK;
}

int ResponseFuture::getOpaque() const {
  return m_opaque;
}

int ResponseFuture::getRequestCode() const {
  return m_requestCode;
}

void ResponseFuture::invokeCompleteCallback() {
  if (m_pCallbackWrap == nullptr) {
    deleteAndZero(m_pResponseCommand);
    return;
  } else {
    m_pCallbackWrap->operationComplete(this, true);
  }
}

void ResponseFuture::invokeExceptionCallback() {
  if (m_pCallbackWrap == nullptr) {
    LOG_ERROR("m_pCallbackWrap is NULL, critical error");
    return;
  } else {
    // here no need retrySendTimes process because of it have timeout
    LOG_ERROR("send msg, callback timeout, opaque:%d, sendTimes:%d, maxRetryTimes:%d", getOpaque(), getRetrySendTimes(),
              getMaxRetrySendTimes());

    m_pCallbackWrap->onException();
  }
}

bool ResponseFuture::isTimeOut() const {
  int64 diff = UtilAll::currentTimeMillis() - m_beginTimestamp;
  //<!only async;
  return m_bAsync && diff > m_timeout;
}

int ResponseFuture::getMaxRetrySendTimes() const {
  return m_maxRetrySendTimes;
}
int ResponseFuture::getRetrySendTimes() const {
  return m_retrySendTimes;
}

void ResponseFuture::setMaxRetrySendTimes(int maxRetryTimes) {
  m_maxRetrySendTimes = maxRetryTimes;
}
void ResponseFuture::setRetrySendTimes(int retryTimes) {
  m_retrySendTimes = retryTimes;
}

void ResponseFuture::setBrokerAddr(const std::string& brokerAddr) {
  m_brokerAddr = brokerAddr;
}

std::string ResponseFuture::getBrokerAddr() const {
  return m_brokerAddr;
}

void ResponseFuture::setRequestCommand(const RemotingCommand& requestCommand) {
  m_requestCommand = requestCommand;
}
const RemotingCommand& ResponseFuture::getRequestCommand() {
  return m_requestCommand;
}

int64 ResponseFuture::leftTime() const {
  int64 diff = UtilAll::currentTimeMillis() - m_beginTimestamp;
  return m_timeout - diff;
}

RemotingCommand* ResponseFuture::getCommand() const {
  return m_pResponseCommand;
}

std::shared_ptr<AsyncCallbackWrap> ResponseFuture::getAsyncCallbackWrap() {
  return m_pCallbackWrap;
}

//<!************************************************************************
}  // namespace rocketmq
