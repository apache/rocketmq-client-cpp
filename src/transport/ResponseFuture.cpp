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
#include "Logging.h"
#include "TcpRemotingClient.h"

namespace rocketmq {
//<!************************************************************************
ResponseFuture::ResponseFuture(int requestCode, int opaque,
                               TcpRemotingClient* powner, int64 timeout,
                               bool bAsync /* = false */,
                               AsyncCallbackWrap* pcall /* = NULL */) {
  m_bAsync.store(bAsync);
  m_requestCode = requestCode;
  m_opaque = opaque;
  m_timeout = timeout;
  m_pCallbackWrap = pcall;
  m_pResponseCommand = NULL;
  m_sendRequestOK = false;
  m_maxRetrySendTimes = 1;
  m_retrySendTimes = 1;
  m_brokerAddr = "";
  m_beginTimestamp = UtilAll::currentTimeMillis();
  m_asyncCallbackStatus = asyncCallBackStatus_init;
  if (getASyncFlag()) {
    m_asyncResponse.store(false);
    m_syncResponse.store(true);
  } else {
    m_asyncResponse.store(true);
    m_syncResponse.store(false);
  }
}

ResponseFuture::~ResponseFuture() {
  deleteAndZero(m_pCallbackWrap);
  /*
    do not set m_pResponseCommand to NULL when destruct, as m_pResponseCommand
    is used by MQClientAPIImpl concurrently, and will be released by producer or
    consumer;
    m_pResponseCommand = NULL;
  */
}

void ResponseFuture::releaseThreadCondition() { m_defaultEvent.notify_all(); }

RemotingCommand* ResponseFuture::waitResponse(int timeoutMillis) {
  boost::unique_lock<boost::mutex> lk(m_defaultEventLock);
  if (!m_defaultEvent.timed_wait(
          lk, boost::posix_time::milliseconds(timeoutMillis))) {
    LOG_WARN("waitResponse of code:%d with opaque:%d timeout", m_requestCode,
             m_opaque);
    m_syncResponse.store(true);
  }
  return m_pResponseCommand;
}

void ResponseFuture::setResponse(RemotingCommand* pResponseCommand) {
  // LOG_DEBUG("setResponse of opaque:%d",m_opaque);
  m_pResponseCommand = pResponseCommand;

  if (!getASyncFlag()) {
    if (m_syncResponse.load() == false) {
      m_defaultEvent.notify_all();
      m_syncResponse.store(true);
    }
  }
}

const bool ResponseFuture::getSyncResponseFlag() {
  if (m_syncResponse.load() == true) {
    return true;
  }
  return false;
}

const bool ResponseFuture::getAsyncResponseFlag() {
  if (m_asyncResponse.load() == true) {
    // LOG_DEBUG("ASYNC flag is TRUE,opaque is:%d",getOpaque() );
    return true;
  }

  return false;
}

void ResponseFuture::setAsyncResponseFlag() { m_asyncResponse.store(true); }

const bool ResponseFuture::getASyncFlag() {
  if (m_bAsync.load() == true) {
    // LOG_DEBUG("ASYNC flag is TRUE,opaque is:%d",getOpaque() );
    return true;
  }
  return false;
}

bool ResponseFuture::isSendRequestOK() { return m_sendRequestOK; }

void ResponseFuture::setSendRequestOK(bool sendRequestOK) {
  m_sendRequestOK = sendRequestOK;
}

int ResponseFuture::getOpaque() const { return m_opaque; }

int ResponseFuture::getRequestCode() const { return m_requestCode; }

void ResponseFuture::setAsyncCallBackStatus(
    asyncCallBackStatus asyncCallbackStatus) {
  boost::lock_guard<boost::mutex> lock(m_asyncCallbackLock);
  if (m_asyncCallbackStatus == asyncCallBackStatus_init) {
    m_asyncCallbackStatus = asyncCallbackStatus;
  }
}

void ResponseFuture::executeInvokeCallback() {
  if (m_pCallbackWrap == NULL) {
    deleteAndZero(m_pResponseCommand);
    return;
  } else {
    if (m_asyncCallbackStatus == asyncCallBackStatus_response) {
      m_pCallbackWrap->operationComplete(this, true);
    } else {
      if (m_pResponseCommand)
        deleteAndZero(m_pResponseCommand);  // the responseCommand from
                                            // RemotingCommand::Decode(mem) will
                                            // only deleted by operationComplete
                                            // automatically
      LOG_WARN(
          "timeout and response incoming concurrently of opaque:%d, and "
          "executeInvokeCallbackException was called earlier",
          m_opaque);
    }
  }
}

void ResponseFuture::executeInvokeCallbackException() {
  if (m_pCallbackWrap == NULL) {
    LOG_ERROR("m_pCallbackWrap is NULL, critical error");
    return;
  } else {
    if (m_asyncCallbackStatus == asyncCallBackStatus_timeout) {

	//here no need retrySendTimes process because of it have timeout
	LOG_ERROR("send msg, callback timeout, opaque:%d, sendTimes:%d, maxRetryTimes:%d", getOpaque(), getRetrySendTimes(), getMaxRetrySendTimes());

      m_pCallbackWrap->onException();
    } else {
      LOG_WARN(
          "timeout and response incoming concurrently of opaque:%d, and "
          "executeInvokeCallback was called earlier",
          m_opaque);
    }
  }
}

bool ResponseFuture::isTimeOut() const {
  int64 diff = UtilAll::currentTimeMillis() - m_beginTimestamp;
  //<!only async;
  return m_bAsync.load() == 1 && diff > m_timeout;
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
void ResponseFuture::setRequestCommand(const RemotingCommand& requestCommand) {
	m_requestCommand = requestCommand;
}

const RemotingCommand& ResponseFuture::getRequestCommand() {
	return m_requestCommand;
}
std::string ResponseFuture::getBrokerAddr() const {
	return m_brokerAddr;
}

int64 ResponseFuture::leftTime() const {
	int64 diff = UtilAll::currentTimeMillis() - m_beginTimestamp;
	return m_timeout - diff;
}

RemotingCommand* ResponseFuture::getCommand() const {
  return m_pResponseCommand;
}

AsyncCallbackWrap* ResponseFuture::getAsyncCallbackWrap() {
  return m_pCallbackWrap;
}

//<!************************************************************************
}  //<!end namespace;
