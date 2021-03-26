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
#ifndef ROCKETMQ_SENDCALLBACK_H_
#define ROCKETMQ_SENDCALLBACK_H_

#include "MQException.h"
#include "SendResult.h"

namespace rocketmq {

enum class SendCallbackType { kSimple, kAutoDelete };

/**
 * SendCallback - callback interface for async send
 */
class ROCKETMQCLIENT_API SendCallback {
 public:
  virtual ~SendCallback() = default;

  virtual void onSuccess(SendResult& sendResult) = 0;
  virtual void onException(MQException& e) noexcept = 0;

  virtual SendCallbackType getSendCallbackType() const { return SendCallbackType::kSimple; }

 public:
  void invokeOnSuccess(SendResult& send_result) noexcept;
  void invokeOnException(MQException& exception) noexcept;
};

/**
 * AutoDeleteSendCallback - callback interface for async send
 *
 * the object of AutoDeleteSendCallback will be deleted automatically by SDK after invoke callback interface
 */
class ROCKETMQCLIENT_API AutoDeleteSendCallback : public SendCallback  // base interface
{
 public:
  SendCallbackType getSendCallbackType() const override final { return SendCallbackType::kAutoDelete; }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_SENDCALLBACK_H_
