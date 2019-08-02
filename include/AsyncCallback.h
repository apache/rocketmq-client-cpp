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
#ifndef __ASYNC_CALLBACK_H__
#define __ASYNC_CALLBACK_H__

#include "MQClientException.h"
#include "PullResult.h"
#include "RocketMQClient.h"
#include "SendResult.h"

namespace rocketmq {

enum SendCallbackType { SEND_CALLBACK_TYPE_SIMPLE = 0, SEND_CALLBACK_TYPE_ATUO_DELETE = 1 };

class ROCKETMQCLIENT_API SendCallback {
 public:
  virtual ~SendCallback() = default;

  virtual void onSuccess(SendResult& sendResult) = 0;
  virtual void onException(MQException& e) noexcept = 0;

  virtual SendCallbackType getSendCallbackType() const { return SEND_CALLBACK_TYPE_SIMPLE; }
};

// async SendCallback will be deleted automatically by rocketmq cpp after invoke callback interface
class ROCKETMQCLIENT_API AutoDeleteSendCallback : public SendCallback {
 public:
  SendCallbackType getSendCallbackType() const override { return SEND_CALLBACK_TYPE_ATUO_DELETE; }
};

enum PullCallbackType { PULL_CALLBACK_TYPE_SIMPLE = 0, PULL_CALLBACK_TYPE_AUTO_DELETE = 1 };

class ROCKETMQCLIENT_API PullCallback {
 public:
  virtual ~PullCallback() = default;

  virtual void onSuccess(MQMessageQueue& mq, PullResult& result, bool bProducePullRequest) = 0;
  virtual void onException(MQException& e) noexcept = 0;

  virtual PullCallbackType getPullCallbackType() const { return PULL_CALLBACK_TYPE_SIMPLE; }
};

class ROCKETMQCLIENT_API AutoDeletePullCallback : public PullCallback {
 public:
  PullCallbackType getPullCallbackType() const override { return PULL_CALLBACK_TYPE_AUTO_DELETE; }
};

}  // namespace rocketmq

#endif  // __ASYNC_CALLBACK_H__
