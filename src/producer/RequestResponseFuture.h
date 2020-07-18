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
#ifndef __REQUEST_RESPONSE_FUTURE__
#define __REQUEST_RESPONSE_FUTURE__

#include <exception>
#include <string>

#include "Message.h"
#include "RequestCallback.h"
#include "concurrent/latch.hpp"

namespace rocketmq {

class RequestResponseFuture {
 public:
  RequestResponseFuture(const std::string& correlationId, long timeoutMillis, RequestCallback* requestCallback);

  void executeRequestCallback() noexcept;

  bool isTimeout();

  MessagePtr waitResponseMessage(int64_t timeout);
  void putResponseMessage(MessagePtr responseMsg);

  bool isSendRequestOk();
  void setSendRequestOk(bool sendRequestOk);

  std::exception_ptr getCause();
  void setCause(std::exception_ptr cause);

 private:
  std::string m_correlationId;
  RequestCallback* m_requestCallback;
  uint64_t m_beginTimestamp;
  MessagePtr m_requestMsg;
  long m_timeoutMillis;
  std::unique_ptr<latch> m_countDownLatch;  // use for synchronization rpc
  MessagePtr m_responseMsg;
  bool m_sendRequestOk;
  std::exception_ptr m_cause;
};

}  // namespace rocketmq

#endif  // __REQUEST_RESPONSE_FUTURE__
