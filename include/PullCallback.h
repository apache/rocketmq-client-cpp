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
#ifndef ROCKETMQ_PULLCALLBACK_H_
#define ROCKETMQ_PULLCALLBACK_H_

#include "MQException.h"
#include "PullResult.h"

namespace rocketmq {

enum class PullCallbackType { kSimple, kAutoDelete };

/**
 * PullCallback - callback interface for async pull
 */
class ROCKETMQCLIENT_API PullCallback {
 public:
  virtual ~PullCallback() = default;

  virtual void onSuccess(std::unique_ptr<PullResult> pull_result) = 0;
  virtual void onException(MQException& e) noexcept = 0;

  virtual PullCallbackType getPullCallbackType() const { return PullCallbackType::kSimple; }

 public:
  void invokeOnSuccess(std::unique_ptr<PullResult> pull_result) noexcept;
  void invokeOnException(MQException& exception) noexcept;
};

/**
 * AutoDeletePullCallback - callback interface for async pull
 *
 * the object of AutoDeletePullCallback will be deleted automatically by SDK after invoke callback interface
 */
class ROCKETMQCLIENT_API AutoDeletePullCallback : public PullCallback {
 public:
  PullCallbackType getPullCallbackType() const override final { return PullCallbackType::kAutoDelete; }
};

}  // namespace rocketmq

#endif  // ROCKETMQ_PULLCALLBACK_H_
