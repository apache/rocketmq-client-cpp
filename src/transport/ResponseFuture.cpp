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

#include "UtilAll.h"

namespace rocketmq {

ResponseFuture::ResponseFuture(int requestCode, int opaque, int64_t timeoutMillis, InvokeCallback* invokeCallback)
    : m_requestCode(requestCode),
      m_opaque(opaque),
      m_timeoutMillis(timeoutMillis),
      m_invokeCallback(invokeCallback),
      m_responseCommand(nullptr),
      m_beginTimestamp(UtilAll::currentTimeMillis()),
      m_sendRequestOK(false),
      m_countDownLatch(nullptr) {
  if (nullptr == invokeCallback) {
    m_countDownLatch = new latch(1);
  }
}

ResponseFuture::~ResponseFuture() {
  delete m_countDownLatch;
  delete m_invokeCallback;

  // do not delete m_pResponseCommand when destruct, as m_responseCommand
  // is used by MQClientAPIImpl concurrently, and will be released by producer or consumer
}

void ResponseFuture::releaseThreadCondition() {
  if (m_countDownLatch != nullptr) {
    m_countDownLatch->count_down();
  }
}

InvokeCallback* ResponseFuture::getInvokeCallback() {
  // if m_invokeCallback is set, this is an async future.
  return m_invokeCallback;
}

void ResponseFuture::releaseInvokeCallback() {
  m_invokeCallback = nullptr;
}

void ResponseFuture::executeInvokeCallback() {
  if (m_invokeCallback != nullptr) {
    m_invokeCallback->operationComplete(this);
  }
}

RemotingCommand* ResponseFuture::waitResponse(int timeoutMillis) {
  if (m_countDownLatch != nullptr) {
    if (timeoutMillis <= 0) {
      timeoutMillis = m_timeoutMillis;
    }
    m_countDownLatch->wait(timeoutMillis, time_unit::milliseconds);
  }
  return m_responseCommand;
}

void ResponseFuture::putResponse(RemotingCommand* responseCommand) {
  m_responseCommand = responseCommand;
  if (m_countDownLatch != nullptr) {
    m_countDownLatch->count_down();
  }
}

RemotingCommand* ResponseFuture::getResponseCommand() const {
  return m_responseCommand;
}

void ResponseFuture::setResponseCommand(RemotingCommand* responseCommand) {
  m_responseCommand = responseCommand;
}

int64_t ResponseFuture::getBeginTimestamp() {
  return m_beginTimestamp;
}

int64_t ResponseFuture::getTimeoutMillis() {
  return m_timeoutMillis;
}

bool ResponseFuture::isTimeout() const {
  auto diff = UtilAll::currentTimeMillis() - m_beginTimestamp;
  return diff > m_timeoutMillis;
}

int64_t ResponseFuture::leftTime() const {
  auto diff = UtilAll::currentTimeMillis() - m_beginTimestamp;
  auto left = m_timeoutMillis - diff;
  return left < 0 ? 0 : left;
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

}  // namespace rocketmq
