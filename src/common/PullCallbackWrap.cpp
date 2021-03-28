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
#include "PullCallbackWrap.h"

#include <cassert>

namespace rocketmq {

void PullCallback::invokeOnSuccess(std::unique_ptr<PullResult> pull_result) noexcept {
  auto type = getPullCallbackType();
  try {
    onSuccess(std::move(pull_result));
  } catch (const std::exception& e) {
    LOG_WARN_NEW("encounter exception when invoke PullCallback::onSuccess(), {}", e.what());
  }
  if (type == PullCallbackType::kAutoDelete) {
    delete this;
  }
}

void PullCallback::invokeOnException(MQException& exception) noexcept {
  auto type = getPullCallbackType();
  try {
    onException(exception);
  } catch (const std::exception& e) {
    LOG_WARN_NEW("encounter exception when invoke PullCallback::onException(), {}", e.what());
  }
  if (type == PullCallbackType::kAutoDelete) {
    delete this;
  }
}

PullCallbackWrap::PullCallbackWrap(PullCallback* pullCallback, MQClientAPIImpl* pClientAPI)
    : pull_callback_(pullCallback), client_api_impl_(pClientAPI) {}

void PullCallbackWrap::operationComplete(ResponseFuture* responseFuture) noexcept {
  std::unique_ptr<RemotingCommand> response(responseFuture->getResponseCommand());  // avoid RemotingCommand leak

  if (pull_callback_ == nullptr) {
    LOG_ERROR("m_pullCallback is NULL, AsyncPull could not continue");
    return;
  }

  if (response != nullptr) {
    try {
      std::unique_ptr<PullResult> pull_result(client_api_impl_->processPullResponse(response.get()));
      assert(pull_result != nullptr);
      pull_callback_->invokeOnSuccess(std::move(pull_result));
    } catch (MQException& e) {
      pull_callback_->invokeOnException(e);
    }
  } else {
    std::string err;
    if (!responseFuture->send_request_ok()) {
      err = "send request failed";
    } else if (responseFuture->isTimeout()) {
      err = "wait response timeout";
    } else {
      err = "unknown reason";
    }
    MQException exception(err, -1, __FILE__, __LINE__);
    pull_callback_->invokeOnException(exception);
  }
}

}  // namespace rocketmq
