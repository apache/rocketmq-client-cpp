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
#pragma once

#include <system_error>

#include "MQClientException.h"
#include "PullResult.h"
#include "SendResult.h"

ROCKETMQ_NAMESPACE_BEGIN

class AsyncCallback {
public:
  virtual ~AsyncCallback() = default;
};

class SendCallback : public AsyncCallback {
public:
  ~SendCallback() override = default;

  virtual void onSuccess(SendResult& send_result) noexcept = 0;

  virtual void onFailure(const std::error_code& ec) noexcept = 0;
};

class PullCallback : public AsyncCallback {
public:
  ~PullCallback() override = default;

  virtual void onSuccess(const PullResult& pull_result) noexcept = 0;

  virtual void onFailure(const std::error_code& ec) noexcept = 0;
};

ROCKETMQ_NAMESPACE_END