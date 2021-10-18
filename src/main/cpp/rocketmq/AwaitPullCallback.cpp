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
#include "AwaitPullCallback.h"

ROCKETMQ_NAMESPACE_BEGIN

void AwaitPullCallback::onSuccess(const PullResult& pull_result) noexcept {
  absl::MutexLock lk(&mtx_);
  completed_ = true;
  // TODO: optimize out messages copy here.
  pull_result_ = pull_result;
  cv_.SignalAll();
}

void AwaitPullCallback::onFailure(const std::error_code& ec) noexcept {
  absl::MutexLock lk(&mtx_);
  completed_ = true;
  ec_ = ec;
  cv_.SignalAll();
}

bool AwaitPullCallback::await() {
  {
    absl::MutexLock lk(&mtx_);
    while (!completed_) {
      cv_.Wait(&mtx_);
    }
    return !hasFailure();
  }
}

ROCKETMQ_NAMESPACE_END