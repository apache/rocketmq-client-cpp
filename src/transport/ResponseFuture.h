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
#ifndef __RESPONSE_FUTURE_H__
#define __RESPONSE_FUTURE_H__

#include "concurrent/latch.hpp"

#include "InvokeCallback.h"
#include "RemotingCommand.h"

namespace rocketmq {

class ResponseFuture {
 public:
  ResponseFuture(int requestCode, int opaque, int64_t timeoutMillis, InvokeCallback* invokeCallback = nullptr);
  virtual ~ResponseFuture();

  void releaseThreadCondition();

  InvokeCallback* getInvokeCallback();
  void releaseInvokeCallback();

  void executeInvokeCallback() noexcept;

  // for sync request
  std::unique_ptr<RemotingCommand> waitResponse(int timeoutMillis = 0);
  void putResponse(std::unique_ptr<RemotingCommand> responseCommand);

  // for async request
  std::unique_ptr<RemotingCommand> getResponseCommand();
  void setResponseCommand(std::unique_ptr<RemotingCommand> responseCommand);

  int64_t getBeginTimestamp();
  int64_t getTimeoutMillis();
  bool isTimeout() const;
  int64_t leftTime() const;

  bool isSendRequestOK() const;
  void setSendRequestOK(bool sendRequestOK = true);

  int getRequestCode() const;
  int getOpaque() const;

 private:
  int m_requestCode;
  int m_opaque;
  int64_t m_timeoutMillis;
  InvokeCallback* m_invokeCallback;

  std::unique_ptr<RemotingCommand> m_responseCommand;

  int64_t m_beginTimestamp;
  bool m_sendRequestOK;

  latch* m_countDownLatch;  // use for synchronization rpc
};

}  // namespace rocketmq

#endif  // __RESPONSE_FUTURE_H__
