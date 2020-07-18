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
#include "RequestResponseFuture.h"

#include "Logging.h"
#include "UtilAll.h"

namespace rocketmq {

RequestResponseFuture::RequestResponseFuture(const std::string& correlationId,
                                             long timeoutMillis,
                                             RequestCallback* requestCallback)
    : m_correlationId(correlationId),
      m_requestCallback(requestCallback),
      m_beginTimestamp(UtilAll::currentTimeMillis()),
      m_requestMsg(nullptr),
      m_timeoutMillis(timeoutMillis),
      m_countDownLatch(nullptr),
      m_responseMsg(nullptr),
      m_sendRequestOk(false),
      m_cause(nullptr) {
  if (nullptr == requestCallback) {
    m_countDownLatch.reset(new latch(1));
  }
}

void RequestResponseFuture::executeRequestCallback() noexcept {
  if (m_requestCallback != nullptr) {
    if (m_sendRequestOk && m_cause == nullptr) {
      try {
        m_requestCallback->onSuccess(std::move(m_responseMsg));
      } catch (const std::exception& e) {
        LOG_WARN_NEW("RequestCallback throw an exception: {}", e.what());
      }
    } else {
      try {
        std::rethrow_exception(m_cause);
      } catch (MQException& e) {
        m_requestCallback->onException(e);
      } catch (const std::exception& e) {
        LOG_WARN_NEW("unexpected exception in RequestResponseFuture: {}", e.what());
      }
    }

    // auto delete callback
    if (m_requestCallback->getRequestCallbackType() == REQUEST_CALLBACK_TYPE_AUTO_DELETE) {
      deleteAndZero(m_requestCallback);
    }
  }
}

bool RequestResponseFuture::isTimeout() {
  auto diff = UtilAll::currentTimeMillis() - m_beginTimestamp;
  return diff > m_timeoutMillis;
}

MessagePtr RequestResponseFuture::waitResponseMessage(int64_t timeout) {
  if (m_countDownLatch != nullptr) {
    if (timeout < 0) {
      timeout = 0;
    }
    m_countDownLatch->wait(timeout, time_unit::milliseconds);
  }
  return m_responseMsg;
}

void RequestResponseFuture::putResponseMessage(MessagePtr responseMsg) {
  m_responseMsg = std::move(responseMsg);
  if (m_countDownLatch != nullptr) {
    m_countDownLatch->count_down();
  }
}

bool RequestResponseFuture::isSendRequestOk() {
  return m_sendRequestOk;
}

void RequestResponseFuture::setSendRequestOk(bool sendRequestOk) {
  m_sendRequestOk = sendRequestOk;
}

std::exception_ptr RequestResponseFuture::getCause() {
  return m_cause;
}

void RequestResponseFuture::setCause(std::exception_ptr cause) {
  m_cause = cause;
}

}  // namespace rocketmq
