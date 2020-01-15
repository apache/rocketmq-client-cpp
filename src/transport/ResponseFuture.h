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
#ifndef __RESPONSEFUTURE_H__
#define __RESPONSEFUTURE_H__

#include <atomic>
#include <condition_variable>

#include "AsyncCallbackWrap.h"
#include "RemotingCommand.h"
#include "UtilAll.h"

namespace rocketmq {

typedef enum AsyncCallbackStatus {
  ASYNC_CALLBACK_STATUS_INIT = 0,
  ASYNC_CALLBACK_STATUS_RESPONSE = 1,
  ASYNC_CALLBACK_STATUS_TIMEOUT = 2
} AsyncCallbAackStatus;

class TcpRemotingClient;
//<!***************************************************************************
class ResponseFuture {
 public:
  ResponseFuture(int requestCode,
                 int opaque,
                 TcpRemotingClient* powner,
                 int64 timeoutMilliseconds,
                 bool bAsync = false,
                 std::shared_ptr<AsyncCallbackWrap> pCallback = std::shared_ptr<AsyncCallbackWrap>());
  virtual ~ResponseFuture();

  void releaseThreadCondition();
  RemotingCommand* waitResponse(int timeoutMillis = 0);
  RemotingCommand* getCommand() const;

  bool setResponse(RemotingCommand* pResponseCommand);

  bool isSendRequestOK() const;
  void setSendRequestOK(bool sendRequestOK);
  int getRequestCode() const;
  int getOpaque() const;

  //<!callback;
  void invokeCompleteCallback();
  void invokeExceptionCallback();
  bool isTimeOut() const;
  int getMaxRetrySendTimes() const;
  int getRetrySendTimes() const;
  int64 leftTime() const;
  const bool getAsyncFlag();
  std::shared_ptr<AsyncCallbackWrap> getAsyncCallbackWrap();

  void setMaxRetrySendTimes(int maxRetryTimes);
  void setRetrySendTimes(int retryTimes);
  void setBrokerAddr(const std::string& brokerAddr);
  void setRequestCommand(const RemotingCommand& requestCommand);
  const RemotingCommand& getRequestCommand();
  std::string getBrokerAddr() const;

 private:
  int m_requestCode;
  int m_opaque;
  int64 m_timeout;  // ms

  const bool m_bAsync;
  std::shared_ptr<AsyncCallbackWrap> m_pCallbackWrap;

  AsyncCallbackStatus m_asyncCallbackStatus;
  std::mutex m_asyncCallbackLock;

  bool m_haveResponse;
  std::mutex m_defaultEventLock;
  std::condition_variable m_defaultEvent;

  int64 m_beginTimestamp;
  bool m_sendRequestOK;
  RemotingCommand* m_pResponseCommand;  //<!delete outside;

  int m_maxRetrySendTimes;
  int m_retrySendTimes;

  std::string m_brokerAddr;
  RemotingCommand m_requestCommand;
};

//<!************************************************************************
}  // namespace rocketmq

#endif
