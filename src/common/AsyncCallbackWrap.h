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

#include "AsyncCallback.h"
#include "InvokeCallback.h"
#include "MQClientAPIImpl.h"
#include "MQMessage.h"
#include "RemotingCommand.h"
#include "ResponseFuture.h"

namespace rocketmq {

class MQClientInstance;
class DefaultMQProducer;
class TopicPublishInfo;

class SendCallbackWrap : public InvokeCallback {
 public:
  SendCallbackWrap(const string& addr,
                   const string& brokerName,
                   const MQMessagePtr msg,
                   RemotingCommand&& request,
                   SendCallback* sendCallback,
                   TopicPublishInfoPtr topicPublishInfo,
                   MQClientInstancePtr instance,
                   int retryTimesWhenSendFailed,
                   int times,
                   DefaultMQProducerPtr producer);

  void operationComplete(ResponseFuture* responseFuture) noexcept override;
  void onExceptionImpl(ResponseFuture* responseFuture, long timeoutMillis, MQException& e, bool needRetry);

  const std::string& getAddr() { return m_addr; }
  const MQMessagePtr getMessage() { return m_msg; }
  RemotingCommand& getRemotingCommand() { return m_request; }

  void setRetrySendTimes(int retrySendTimes) { m_times = retrySendTimes; }
  int getRetrySendTimes() { return m_times; }
  int getMaxRetrySendTimes() { return m_timesTotal; }

 private:
  std::string m_addr;
  std::string m_brokerName;
  const MQMessagePtr m_msg;
  RemotingCommand m_request;
  SendCallback* m_sendCallback;
  TopicPublishInfoPtr m_topicPublishInfo;
  MQClientInstancePtr m_instance;
  int m_timesTotal;
  int m_times;
  std::weak_ptr<DefaultMQProducer> m_producer;  // FIXME: ensure object is live.
};

class PullCallbackWrap : public InvokeCallback {
 public:
  PullCallbackWrap(PullCallback* pullCallback, MQClientAPIImpl* pClientAPI);

  void operationComplete(ResponseFuture* responseFuture) noexcept override;

 private:
  PullCallback* m_pullCallback;
  MQClientAPIImpl* m_pClientAPI;
};

}  // namespace rocketmq

#endif  // __ASYNC_CALLBACK_WRAP_H__
