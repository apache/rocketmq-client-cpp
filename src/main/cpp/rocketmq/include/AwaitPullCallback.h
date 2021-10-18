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

#include "absl/synchronization/mutex.h"

#include "rocketmq/AsyncCallback.h"

ROCKETMQ_NAMESPACE_BEGIN

class AwaitPullCallback : public PullCallback {
public:
  explicit AwaitPullCallback(PullResult& pull_result) : pull_result_(pull_result) {
  }

  void onSuccess(const PullResult& pull_result) noexcept override;

  void onFailure(const std::error_code& ec) noexcept override;

  bool await();

  bool hasFailure() const {
    return ec_.operator bool();
  }

  bool isCompleted() const {
    return completed_;
  }

  const std::error_code& errorCode() const noexcept {
    return ec_;
  }

private:
  PullResult& pull_result_;
  absl::Mutex mtx_;
  absl::CondVar cv_;
  bool completed_{false};
  std::error_code ec_;
};

ROCKETMQ_NAMESPACE_END