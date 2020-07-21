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
#ifndef ROCKETMQ_COMMON_SENDCALLBACKWRAP_H_
#define ROCKETMQ_COMMON_SENDCALLBACKWRAP_H_

#include <functional>

#include "DefaultMQProducerImpl.h"
#include "InvokeCallback.h"
#include "MQClientInstance.h"
#include "Message.h"
#include "RemotingCommand.h"
#include "ResponseFuture.h"
#include "SendCallback.h"
#include "TopicPublishInfo.hpp"

namespace rocketmq {

class SendCallbackWrap : public InvokeCallback {
 public:
  SendCallbackWrap(const std::string& addr,
                   const std::string& brokerName,
                   const MessagePtr msg,
                   RemotingCommand&& request,
                   SendCallback* sendCallback,
                   TopicPublishInfoPtr topicPublishInfo,
                   MQClientInstancePtr instance,
                   int retryTimesWhenSendFailed,
                   int times,
                   DefaultMQProducerImplPtr producer);

  void operationComplete(ResponseFuture* responseFuture) noexcept override;
  void onExceptionImpl(ResponseFuture* responseFuture, long timeoutMillis, MQException& e, bool needRetry);

  const std::string& getAddr() { return addr_; }
  const MessagePtr getMessage() { return msg_; }
  RemotingCommand& getRemotingCommand() { return request_; }

  void setRetrySendTimes(int retrySendTimes) { times_ = retrySendTimes; }
  int getRetrySendTimes() { return times_; }
  int getMaxRetrySendTimes() { return times_total_; }

 private:
  std::string addr_;
  std::string broker_name_;
  const MessagePtr msg_;
  RemotingCommand request_;
  SendCallback* send_callback_;
  TopicPublishInfoPtr topic_publish_info_;
  MQClientInstancePtr instance_;
  int times_total_;
  int times_;
  std::weak_ptr<DefaultMQProducerImpl> producer_;
};

}  // namespace rocketmq

#endif  // ROCKETMQ_COMMON_SENDCALLBACKWRAP_H_
