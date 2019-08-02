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
#ifndef __ASYNC_CALLBACK_WRAP_H__
#define __ASYNC_CALLBACK_WRAP_H__

#include <functional>

#include "AsyncArg.h"
#include "AsyncCallback.h"
#include "InvokeCallback.h"
#include "MQClientAPIImpl.h"
#include "MQMessage.h"
#include "RemotingCommand.h"
#include "ResponseFuture.h"

namespace rocketmq {

class SendCallbackWrap : public InvokeCallback {
 public:
  typedef std::function<void(SendCallbackWrap*, int64)> SendMessageDelegate;  // throw(MQClientException)

  SendCallbackWrap(const std::string& addr,
                   const std::string& brokerName,
                   const MQMessage& msg,
                   const RemotingCommand& requestCommand,
                   int maxRetrySendTimes,
                   int retrySendTimes,
                   SendCallback* pSendCallback,
                   MQClientAPIImpl* pClientAPI,
                   SendMessageDelegate retrySendDelegate);

  void operationComplete(ResponseFuture* responseFuture) noexcept override;
  void onExceptionImpl(ResponseFuture* responseFuture);

  const std::string& getAddr() { return m_addr; }
  const MQMessage& getMessage() { return m_msg; }
  const RemotingCommand& getRemotingCommand() { return m_requestCommand; }

  void setRetrySendTimes(int retrySendTimes) { m_retrySendTimes = retrySendTimes; }
  int getRetrySendTimes() { return m_retrySendTimes; }
  int getMaxRetrySendTimes() { return m_maxRetrySendTimes; }

 private:
  SendCallback* m_pSendCallback;
  MQClientAPIImpl* m_pClientAPI;

  std::string m_addr;
  std::string m_brokerName;

  MQMessage m_msg;
  RemotingCommand m_requestCommand;

  int m_retrySendTimes;
  int m_maxRetrySendTimes;

  SendMessageDelegate m_retrySendDelegate;
};

class PullCallbackWrap : public InvokeCallback {
 public:
  PullCallbackWrap(PullCallback* pPullCallback, MQClientAPIImpl* pClientAPI, void* pArg);

  void operationComplete(ResponseFuture* responseFuture) noexcept override;

 private:
  PullCallback* m_pPullCallback;
  MQClientAPIImpl* m_pClientAPI;

  AsyncArg m_pArg;
};

}  // namespace rocketmq

#endif  // __ASYNC_CALLBACK_WRAP_H__
